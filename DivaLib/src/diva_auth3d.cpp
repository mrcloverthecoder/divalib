#include "pch.h"
#include "diva_auth3d.h"

using namespace Auth;

const static char Signature[] = "#A3DA__________\n# date time was eliminated.\n";

namespace Auth
{
	static void WriteProperty1D(std::string_view propName, Property::CanonicalProperties& prop, const Property1D& data)
	{
		if (!prop.OpenScope(propName))
			return;

		constexpr int32_t bufferSize = 0x40;
		char buffer[bufferSize] = { '\0' };

		prop.Add("type", data.Type);
		if (data.Type == KEY_TYPE_NONE)
		{
			prop.CloseScope();
			return;
		}
		else if (data.Type == KEY_TYPE_STATIC)
		{
			prop.Add("value", data.Value);
			prop.CloseScope();
			return;
		}

		int32_t keyLength = static_cast<int32_t>(data.Keys.size());
		prop.Add("key.length", keyLength);

		if (keyLength > 0)
			prop.Add("max", data.Max);

		for (int32_t i = 0; i < keyLength; i++)
		{
			const auto& key = data.Keys[i];

			sprintf_s(buffer, bufferSize, "key.%d", i);
			// key.%d
			prop.OpenScope(buffer);
			{
				prop.Add("type", key.Type);
				switch (key.Type)
				{
				case KEY_TYPE_NONE:
					sprintf_s(buffer, bufferSize, "(%f)", key.Frame);
					break;
				case KEY_TYPE_STATIC:
					sprintf_s(buffer, bufferSize, "(%f,%f)", key.Frame, key.Value);
					break;
				case KEY_TYPE_LINEAR:
					sprintf_s(buffer, bufferSize, "(%f,%f,%f)", key.Frame, key.Value, key.T1);
					break;
				case KEY_TYPE_HERMITE:
					sprintf_s(buffer, bufferSize, "(%f,%f,%f,%f)", key.Frame, key.Value, key.T1, key.T2);
					break;
				}
				prop.Add("data", buffer);
			}
			prop.CloseScope();
		}

		prop.CloseScope();
	}

	static void WriteProperty3D(std::string_view propName, Property::CanonicalProperties& prop, const Property3D& data)
	{
		if (!prop.OpenScope(propName))
			return;

		WriteProperty1D("x", prop, data.X);
		WriteProperty1D("y", prop, data.Y);
		WriteProperty1D("z", prop, data.Z);

		prop.CloseScope();
	}

	static void WriteCameraRoot(Property::CanonicalProperties& prop, const CameraRoot& cam)
	{
		WriteProperty3D("trans", prop, cam.Translation);
		WriteProperty3D("rot", prop, cam.Rotation);
		WriteProperty3D("scale", prop, cam.Scale);
		WriteProperty1D("visibility", prop, cam.Visibility);

		// ViewPoint
		prop.OpenScope("view_point");
		{
			prop.Add("aspect", cam.ViewPoint.Aspect);
			prop.Add("fov_is_horizontal", cam.ViewPoint.FoVIsHorizontal);
			WriteProperty1D("fov", prop, cam.ViewPoint.FoV);
			WriteProperty3D("trans", prop, cam.ViewPoint.Translation);
			WriteProperty3D("rot", prop, cam.ViewPoint.Rotation);
			WriteProperty3D("scale", prop, cam.ViewPoint.Scale);
			WriteProperty1D("visibility", prop, cam.ViewPoint.Visibility);
		}
		prop.CloseScope();

		// Interest
		prop.OpenScope("interest");
		{
			WriteProperty3D("trans", prop, cam.Interest.Translation);
			WriteProperty3D("rot", prop, cam.Interest.Rotation);
			WriteProperty3D("scale", prop, cam.Interest.Scale);
			WriteProperty1D("visibility", prop, cam.Interest.Visibility);
		}
		prop.CloseScope();
	}

	static void WriteHrcNode(Property::CanonicalProperties& prop, const HrcNode& hrc)
	{
		prop.Add("name", hrc.Name);
		prop.Add("parent", hrc.Parent);
		WriteProperty3D("trans", prop, hrc.Translation);
		WriteProperty3D("rot", prop, hrc.Rotation);
		WriteProperty3D("scale", prop, hrc.Scale);
		WriteProperty1D("visibility", prop, hrc.Visibility);
	}

	static void WriteObjectHrc(Property::CanonicalProperties& prop, const ObjectHrc& hrc)
	{
		char buffer[0x40] = { '\0' };

		prop.Add("name", hrc.Name);
		prop.Add("uid_name", hrc.UIDName);
		prop.Add("shadow", hrc.Shadow);

		prop.Add("node.length", static_cast<int32_t>(hrc.Nodes.size()));
		for (size_t i = 0; i < hrc.Nodes.size(); i++)
		{
			sprintf_s(buffer, 0x40, "node.%d", static_cast<int32_t>(i));
			prop.OpenScope(buffer);
			WriteHrcNode(prop, hrc.Nodes[i]);
			prop.CloseScope();
		}
	}

	static void WriteObject(Property::CanonicalProperties& prop, const Object& obj)
	{
		prop.Add("name", obj.Name);
		prop.Add("uid_name", obj.UIDName);
		WriteProperty3D("trans", prop, obj.Translation);
		WriteProperty3D("rot", prop, obj.Rotation);
		WriteProperty3D("scale", prop, obj.Scale);
		WriteProperty1D("visibility", prop, obj.Visibility);
	}

	template <typename T>
	static void WriteList(Property::CanonicalProperties& prop, std::string_view name, std::vector<T>& data)
	{
		if (data.size() < 1)
			return;

		char buffer[0x40] = { '\0' };
		sprintf_s(buffer, 0x40, "%s.length", name.data());

		prop.Add(buffer, data.size());
		for (size_t i = 0; i < data.size(); i++)
		{
			sprintf_s(buffer, 0x40, "%s.%zu", name.data(), i);
			prop.Add(buffer, data[i]);
		}
	}

	static void WriteInfoAndPlayControl(Property::CanonicalProperties& prop, Auth3D& auth)
	{
		if (auth.CompressF16 != Auth::CompressF16::No)
			prop.Add("_.compress_f16", static_cast<int32_t>(auth.CompressF16));
		prop.Add("_.converter.version", auth.ConverterVersion);
		prop.Add("_.property.version", auth.PropertyVersion);
		prop.Add("_.file_name", auth.Filename);

		prop.Add("play_control.begin", auth.PlayControl.Begin);
		prop.Add("play_control.fps", auth.PlayControl.Framerate);
		prop.Add("play_control.size", auth.PlayControl.Size > 0.0f ? auth.PlayControl.Size : auth.GetMaxFrame());
	}
}

bool Auth3D::Write(IO::Writer& writer)
{
	constexpr int32_t bufferSize = 0x100;
	char buffer[bufferSize] = { '\0' };

	Property::CanonicalProperties prop;
	Auth::WriteInfoAndPlayControl(prop, *this);
	
	if (Cameras.size() > 0)
	{
		prop.Add("camera_root.length", static_cast<int32_t>(Cameras.size()));
		for (size_t i = 0; i < Cameras.size(); i++)
		{
			sprintf_s(buffer, bufferSize, "camera_root.%d", static_cast<int32_t>(i));
			prop.OpenScope(buffer);
			Auth::WriteCameraRoot(prop, Cameras[i]);
			prop.CloseScope();
		}
	}

	if (ObjectHrcs.size() > 0)
	{
		prop.Add("objhrc.length", static_cast<int32_t>(ObjectHrcs.size()));
		for (size_t i = 0; i < ObjectHrcs.size(); i++)
		{
			sprintf_s(buffer, bufferSize, "objhrc.%d", static_cast<int32_t>(i));
			prop.OpenScope(buffer);
			Auth::WriteObjectHrc(prop, ObjectHrcs[i]);
			prop.CloseScope();
		}
	}

	prop.Add("object.length", static_cast<int32_t>(Objects.size()));
	prop.Add("object_list.length", static_cast<int32_t>(ObjectList.size()));
	for (size_t i = 0; i < Objects.size(); i++)
	{
		sprintf_s(buffer, bufferSize, "object.%d", static_cast<int32_t>(i));
		prop.OpenScope(buffer);
		Auth::WriteObject(prop, Objects[i]);
		prop.CloseScope();
	}

	for (size_t i = 0; i < ObjectList.size(); i++)
	{
		sprintf_s(buffer, bufferSize, "object_list.%d", static_cast<int32_t>(i));
		prop.Add(buffer, ObjectList[i]);
	}

	writer.Write(Signature, 44);
	prop.Write(writer);
	return true;
}

namespace AuthCompressed
{
	void WriteProperty1D(IO::Writer& bin, Auth::Property1D& prop, Auth::CompressF16 compress)
	{
		bin.ScheduleWriteOffset([&prop, compress](IO::Writer& bin)
		{
			switch (prop.Type)
			{
			case Auth::KEY_TYPE_NONE:
				bin.WriteInt32(0);
				bin.WriteInt32(0);
				return;
			case Auth::KEY_TYPE_STATIC:
				bin.WriteInt32(0x01);
				bin.WriteFloat32(prop.Value);
				return;
			default:
				bin.WriteChar(static_cast<char>(prop.Type));
				bin.WriteChar(0); // Pre/Post-Infinity type bitmask (0~3 - Pre, 4~7 - Post)
				bin.WriteChar(0);
				bin.WriteChar(0);
				bin.WriteInt32(0);
				bin.WriteFloat32(prop.Max);
				bin.WriteInt32(static_cast<int32_t>(prop.Keys.size()));
				break;
			}

			for (size_t i = 0; i < prop.Keys.size(); i++)
			{
				auto& key = prop.Keys[i];

				switch (compress)
				{
				case Auth::CompressF16::No:
					bin.WriteFloat32(key.Frame);
					bin.WriteFloat32(key.Value);
					bin.WriteFloat32(key.T1);
					bin.WriteFloat32(key.T2);
					break;
				case Auth::CompressF16::Normal:
					bin.WriteUInt16(static_cast<uint16_t>(key.Frame));
					bin.WriteFloat16(key.Value);
					bin.WriteFloat32(key.T1);
					bin.WriteFloat32(key.T2);
					break;
				case Auth::CompressF16::Compact:
					bin.WriteUInt16(static_cast<uint16_t>(key.Frame));
					bin.WriteFloat16(key.Value);
					bin.WriteFloat16(key.T1);
					bin.WriteFloat16(key.T2);
					break;
				}
			}
		});
	}

	void WriteProperty3D(IO::Writer& bin, Auth::Property3D& prop, Auth::CompressF16 compression)
	{
		WriteProperty1D(bin, prop.X, compression);
		WriteProperty1D(bin, prop.Y, compression);
		WriteProperty1D(bin, prop.Z, compression);
	}

	void WriteModelTransform(IO::Writer& bin, Auth::Property3D& trans, Auth::Property3D& rot, Auth::Property3D& scale, Auth::Property1D& visibility, Auth::CompressF16 compress)
	{
		WriteProperty3D(bin, scale, Auth::CompressF16::No);
		WriteProperty3D(bin, rot, compress);
		WriteProperty3D(bin, trans, Auth::CompressF16::No);
		WriteProperty1D(bin, visibility, Auth::CompressF16::No);
		bin.WriteInt32(0);
		bin.WriteInt32(0);
	}

	void WriteHrcNode(Property::CanonicalProperties& prop, IO::Writer& bin, Auth::HrcNode& node, Auth::CompressF16 compress)
	{
		prop.Add("name", node.Name);
		prop.Add("parent", node.Parent);
		prop.Add("model_transform.bin_offset", bin.GetPosition());
		WriteModelTransform(bin, node.Translation, node.Rotation, node.Scale, node.Visibility, compress);
	}

	void WriteObjectHrc(Property::CanonicalProperties& prop, IO::Writer& bin, Auth::ObjectHrc& hrc, Auth::CompressF16 compress)
	{
		char buffer[0x40] = { '\0' };

		prop.Add("name", hrc.Name);
		prop.Add("uid_name", hrc.UIDName);
		prop.Add("shadow", hrc.Shadow);
		prop.Add("node.length", hrc.Nodes.size());
		for (size_t i = 0; i < hrc.Nodes.size(); i++)
		{
			auto& node = hrc.Nodes[i];
			sprintf_s(buffer, 0x40, "node.%zu", i);
			prop.OpenScope(buffer);
			WriteHrcNode(prop, bin, node, compress);
			prop.CloseScope();
		}
	}
}

bool Auth3D::WriteCompressed(IO::Writer& destination)
{
	// NOTE: Create text and binary section writers
	Property::CanonicalProperties prop;
	IO::Writer binSection;

	// NOTE: Write A3DC data
	char buffer[0x40] = { '\0' };

	Auth::WriteInfoAndPlayControl(prop, *this);

	prop.Add("objhrc.length", ObjectHrcs.size());
	for (size_t i = 0; i < ObjectHrcs.size(); i++)
	{
		auto& hrc = ObjectHrcs[i];
		sprintf_s(buffer, 0x40, "objhrc.%zu", i);
		prop.OpenScope(buffer);
		AuthCompressed::WriteObjectHrc(prop, binSection, hrc, CompressF16);
		prop.CloseScope();
	}

	Auth::WriteList(prop, "objhrc_list", ObjectHrcList);

	// NOTE: Flush A3DC data to destination
	// NOTE: Write top-most header
	const char* const signature = "#A3DC__________\n";
	const uint8_t headerData[] = { 0, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20, 0, 0x02, 0, 0x10 };
	destination.Write(signature, 0x10);
	destination.Write(headerData, 0x10);

	// NOTE: Write second header
	destination.SetEndianness(IO::Endianness::Big);
	destination.WriteInt32(0x50000000);
	destination.ScheduleWriteOffsetAndSize([&prop](IO::Writer& writer) { prop.Write(writer); });
	destination.ScheduleWrite([](IO::Writer& writer) { writer.Pad(32); });
	destination.WriteInt32(0x01);
	destination.WriteInt32(0x424C0000);
	destination.ScheduleWriteOffsetAndSize([&binSection](IO::Writer& writer)
	{
		binSection.FlushScheduledWrites();
		binSection.CopyTo(writer);
		writer.Pad(32);
	});
	destination.WriteInt32(0x20);
	destination.FlushScheduledWrites();

	return true;
}