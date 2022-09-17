#include "pch.h"
#include "diva_auth3d.h"

using namespace Auth;

constexpr static char Signature[] = "#A3DA__________\n# date time was eliminated.\n";

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
}

bool Auth3D::Write(IO::Writer& writer)
{
	constexpr int32_t bufferSize = 0x100;
	char buffer[bufferSize] = { '\0' };

	Property::CanonicalProperties prop;
	
	prop.Add("play_control.begin", PlayControl.Begin);
	prop.Add("play_control.fps", PlayControl.Framerate);
	prop.Add("play_control.size", PlayControl.Size);
	prop.Add("_.converter.version", ConverterVersion);
	prop.Add("_.property.version", PropertyVersion);
	prop.Add("_.file_name", Filename);
	
	prop.Add("camera_root.length", static_cast<int32_t>(Cameras.size()));
	for (size_t i = 0; i < Cameras.size(); i++)
	{
		sprintf_s(buffer, bufferSize, "camera_root.%d", static_cast<int32_t>(i));
		prop.OpenScope(buffer);
		Auth::WriteCameraRoot(prop, Cameras[i]);
		prop.CloseScope();
	}

	writer.Write(Signature, 44);
	prop.Write(writer);
	return true;
}