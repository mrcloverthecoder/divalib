#include "pch.h"
#include "diva_auth2d.h"

namespace Aet
{
	using ItemReference = std::pair<uint64_t, void*>;

	static void FixItemReferences(Aet::Scene& scene, const std::vector<ItemReference>& itemReferences)
	{
		// On a side note, this kind of indent is cursed AF
		for (Aet::Composition& comp : scene.Compositions)
			for (Aet::Layer& layer : comp.Layers)
				for (const auto& pair : itemReferences)
				{
					// uint64_t so the compiler complains less on x64 build target
					if (pair.first == (uint64_t)layer.Item)
					{
						layer.Item = pair.second;
						break;
					}
				}
	}

	static void ReadProperty1D(IO::Stream& stream, Aet::Property1D& prop)
	{
		int32_t keyCount = stream.ReadInt32();
		uint32_t keyOffset = stream.ReadUInt32();

		if (keyCount < 1 || keyOffset == 0)
			return;

		stream.ExecuteAtOffset(keyOffset, [&]
		{
			if (keyCount == 1)
				prop.Keyframes.emplace_back(0.0f, stream.ReadFloat32(), 0.0f);
			else
			{
				for (int i = 0; i < keyCount; i++)
					prop.Keyframes.emplace_back(stream.ReadFloat32(), 0.0f, 0.0f);

				for (Aet::Keyframe1D& key : prop.Keyframes)
				{
					key.Value = stream.ReadFloat32();
					key.Tangent = stream.ReadFloat32();
				}
			}
		});
	}

	static void ReadLayerVideo(IO::Stream& stream, Aet::LayerVideo& video)
	{
		video.TransferMode.BlendMode = (Aet::BlendMode)stream.ReadUInt8();
		video.TransferMode.TrackMatte = (Aet::TrackMatteMode)stream.ReadUInt8();
		stream.Read(&video.TransferMode.Flags, sizeof(uint8_t));
		stream.SeekCurrent(1);
		ReadProperty1D(stream, video.AnchorX);
		ReadProperty1D(stream, video.AnchorY);
		ReadProperty1D(stream, video.PositionX);
		ReadProperty1D(stream, video.PositionY);
		ReadProperty1D(stream, video.Rotation);
		ReadProperty1D(stream, video.ScaleX);
		ReadProperty1D(stream, video.ScaleY);
		ReadProperty1D(stream, video.Opacity);
	}

	static void ReadLayer(IO::Stream& stream, Aet::Layer& layer)
	{
		stream.ReadNullStringOffset(layer.Name);
		layer.StartTime = stream.ReadFloat32();
		layer.EndTime = stream.ReadFloat32();
		layer.OffsetTime = stream.ReadFloat32();
		layer.TimeScale = stream.ReadFloat32();
		stream.Read(&layer.Flags, sizeof(uint16_t));
		layer.Quality = (Aet::Quality)stream.ReadUInt8();
		layer.ItemType = (Aet::ItemType)stream.ReadUInt8();
		layer.Item = (void*)stream.ReadUInt32();
		stream.ReadUInt32(); // Parent offset

		int32_t markerCount = stream.ReadInt32();
		uint32_t markerOffset = stream.ReadUInt32();
		uint32_t videoOffset = stream.ReadUInt32();
		uint32_t audioOffset = stream.ReadUInt32();

		stream.ExecuteAtOffset(videoOffset, [&] { ReadLayerVideo(stream, layer.Video); });
	};

	static void ReadComposition(IO::Stream& stream, Aet::Composition& comp)
	{
		int32_t layerCount = stream.ReadInt32();
		uint32_t layerOffset = stream.ReadUInt32();

		stream.ExecuteAtOffset(layerOffset, [&]
		{
			for (int i = 0; i < layerCount; i++)
			{
				Aet::Layer& layer = comp.Layers.emplace_back();
				Aet::ReadLayer(stream, layer);
			}
		});
	};

	static void ReadVideo(IO::Stream& stream, Aet::Video& video)
	{
		stream.Read(&video.Color, 0x04);
		video.Width = stream.ReadUInt16();
		video.Height = stream.ReadUInt16();
		video.Frames = stream.ReadFloat32();

		int32_t srcCount = stream.ReadInt32();
		uint32_t srcOffset = stream.ReadUInt32();

		// Read video sources
		stream.ExecuteAtOffset(srcOffset, [&]
		{
			for (int i = 0; i < srcCount; i++)
			{
				// Create video source
				Aet::VideoSrc& src = video.Sources.emplace_back();
				// Read data from stream
				stream.ReadNullStringOffset(src.Name);
				src.Id = stream.ReadUInt32();
			}
		});
	}

	static void ReadScene(IO::Stream& stream, Aet::Scene& scene)
	{
		stream.ReadNullStringOffset(scene.Name);
		scene.StartFrame = stream.ReadFloat32();
		scene.EndFrame = stream.ReadFloat32();
		scene.Framerate = stream.ReadFloat32();
		stream.Read(scene.BackgroundColor, 4);
		scene.Width = stream.ReadInt32();
		scene.Height = stream.ReadInt32();

		uint32_t cameraOffset = stream.ReadUInt32();
		int32_t compCount = stream.ReadInt32();
		uint32_t compOffset = stream.ReadUInt32();
		int32_t videoCount = stream.ReadInt32();
		uint32_t videoOffset = stream.ReadUInt32();
		int32_t audioCount = stream.ReadInt32();
		uint32_t audioOffset = stream.ReadUInt32();

		std::vector<Aet::ItemReference> itemReferences;

		stream.ExecuteAtOffset(compOffset, [&]
		{
			for (int i = 0; i < compCount; i++)
			{
				// Create new composition
				Aet::Composition& comp = scene.Compositions.emplace_back();
				// Store reference
				itemReferences.push_back(std::make_pair<uint64_t, void*>(stream.Tell(), &comp));
				// Read data from stream
				Aet::ReadComposition(stream, comp);
			}
		});

		stream.ExecuteAtOffset(videoOffset, [&]
		{
			for (int i = 0; i < videoCount; i++)
			{
				// Create new video
				Aet::Video& video = scene.Videos.emplace_back();
				// Store reference
				itemReferences.push_back(std::make_pair<uint64_t, void*>(stream.Tell(), &video));
				// Read data from stream
				Aet::ReadVideo(stream, video);
			}
		});

		// Resolve item references
		Aet::FixItemReferences(scene, itemReferences);
	}
}

void Aet::AetSet::Parse(IO::Stream& stream)
{
	uint32_t offset = 0;
	while (offset = stream.ReadInt32(), offset != 0)
	{
		stream.ExecuteAtOffset(offset, [&stream, this]
		{
			Aet::Scene& scene = Scenes.emplace_back();
			Aet::ReadScene(stream, scene);
		});
	}
}