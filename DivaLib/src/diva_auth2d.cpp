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

	static void ReadProperty1D(IO::Reader& reader, Aet::Property1D& prop)
	{
		int32_t keyCount = reader.ReadInt32();
		uint32_t keyOffset = reader.ReadUInt32();

		if (keyCount < 1 || keyOffset == 0)
			return;

		reader.ExecuteAtOffset(keyOffset, [&]
		{
			if (keyCount == 1)
				prop.Keyframes.emplace_back(0.0f, reader.ReadFloat32(), 0.0f);
			else
			{
				for (int i = 0; i < keyCount; i++)
					prop.Keyframes.emplace_back(reader.ReadFloat32(), 0.0f, 0.0f);

				for (Aet::Keyframe1D& key : prop.Keyframes)
				{
					key.Value = reader.ReadFloat32();
					key.Tangent = reader.ReadFloat32();
				}
			}
		});
	}

	static void ReadLayerVideo(IO::Reader& reader, Aet::LayerVideo& video)
	{
		video.TransferMode.BlendMode = (Aet::BlendMode)reader.ReadUInt8();
		video.TransferMode.TrackMatte = (Aet::TrackMatteMode)reader.ReadUInt8();
		reader.Read(&video.TransferMode.Flags, sizeof(uint8_t));
		reader.SeekCurrent(1);
		ReadProperty1D(reader, video.AnchorX);
		ReadProperty1D(reader, video.AnchorY);
		ReadProperty1D(reader, video.PositionX);
		ReadProperty1D(reader, video.PositionY);
		ReadProperty1D(reader, video.Rotation);
		ReadProperty1D(reader, video.ScaleX);
		ReadProperty1D(reader, video.ScaleY);
		ReadProperty1D(reader, video.Opacity);
	}

	static void ReadLayer(IO::Reader& reader, Aet::Layer& layer)
	{
		layer.Name = reader.ReadStringOffset();
		layer.StartTime = reader.ReadFloat32();
		layer.EndTime = reader.ReadFloat32();
		layer.OffsetTime = reader.ReadFloat32();
		layer.TimeScale = reader.ReadFloat32();
		reader.Read(&layer.Flags, sizeof(uint16_t));
		layer.Quality = (Aet::Quality)reader.ReadUInt8();
		layer.ItemType = (Aet::ItemType)reader.ReadUInt8();
		layer.Item = (void*)reader.ReadUInt32();
		reader.ReadUInt32(); // Parent offset

		int32_t markerCount = reader.ReadInt32();
		uint32_t markerOffset = reader.ReadUInt32();
		uint32_t videoOffset = reader.ReadUInt32();
		uint32_t audioOffset = reader.ReadUInt32();

		reader.ExecuteAtOffset(videoOffset, [&] { ReadLayerVideo(reader, layer.Video); });
	}

	static void ReadComposition(IO::Reader& reader, Aet::Composition& comp)
	{
		int32_t layerCount = reader.ReadInt32();
		uint32_t layerOffset = reader.ReadUInt32();

		reader.ExecuteAtOffset(layerOffset, [&]
		{
			for (int i = 0; i < layerCount; i++)
			{
				Aet::Layer& layer = comp.Layers.emplace_back();
				Aet::ReadLayer(reader, layer);
			}
		});
	}

	static void ReadVideo(IO::Reader& reader, Aet::Video& video)
	{
		reader.Read(&video.Color, 0x04);
		video.Width = reader.ReadUInt16();
		video.Height = reader.ReadUInt16();
		video.Frames = reader.ReadFloat32();

		int32_t srcCount = reader.ReadInt32();
		uint32_t srcOffset = reader.ReadUInt32();

		// Read video sources
		reader.ExecuteAtOffset(srcOffset, [&]
		{
			for (int i = 0; i < srcCount; i++)
			{
				// Create video source
				Aet::VideoSrc& src = video.Sources.emplace_back();
				// Read data from reader
				src.Name = reader.ReadStringOffset();
				src.Id = reader.ReadUInt32();
			}
		});
	}

	static void ReadScene(IO::Reader& reader, Aet::Scene& scene)
	{
		scene.Name = reader.ReadStringOffset();
		scene.StartFrame = reader.ReadFloat32();
		scene.EndFrame = reader.ReadFloat32();
		scene.Framerate = reader.ReadFloat32();
		reader.Read(scene.BackgroundColor, 4);
		scene.Width = reader.ReadInt32();
		scene.Height = reader.ReadInt32();

		uint32_t cameraOffset = reader.ReadUInt32();
		int32_t compCount = reader.ReadInt32();
		uint32_t compOffset = reader.ReadUInt32();
		int32_t videoCount = reader.ReadInt32();
		uint32_t videoOffset = reader.ReadUInt32();
		int32_t audioCount = reader.ReadInt32();
		uint32_t audioOffset = reader.ReadUInt32();

		std::vector<Aet::ItemReference> itemReferences;

		reader.ExecuteAtOffset(compOffset, [&]
		{
			for (int i = 0; i < compCount; i++)
			{
				// Create new composition
				Aet::Composition& comp = scene.Compositions.emplace_back();
				// Store reference
				itemReferences.push_back(std::make_pair<uint64_t, void*>(reader.GetPosition(), &comp));
				// Read data from reader
				Aet::ReadComposition(reader, comp);
			}
		});

		reader.ExecuteAtOffset(videoOffset, [&]
		{
			for (int i = 0; i < videoCount; i++)
			{
				// Create new video
				Aet::Video& video = scene.Videos.emplace_back();
				// Store reference
				itemReferences.push_back(std::make_pair<uint64_t, void*>(reader.GetPosition(), &video));
				// Read data from reader
				Aet::ReadVideo(reader, video);
			}
		});

		// Resolve item references
		Aet::FixItemReferences(scene, itemReferences);
	}
}

void Aet::AetSet::Parse(IO::Reader& reader)
{
	uint32_t offset = 0;
	while (offset = reader.ReadInt32(), offset != 0)
	{
		reader.ExecuteAtOffset(offset, [&reader, this]
		{
			Aet::Scene& scene = Scenes.emplace_back();
			Aet::ReadScene(reader, scene);
		});
	}
}