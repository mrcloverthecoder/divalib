#pragma once

#include <string>
#include <vector>
#include "diva_prop.h"

#define SCALE_DEFAULT { { 1, 1.0f }, { 1, 1.0f }, { 1, 1.0f } }

namespace Auth
{
	enum KeyType : int32_t
	{
		KEY_TYPE_NONE = 0,
		KEY_TYPE_STATIC,
		KEY_TYPE_LINEAR,
		KEY_TYPE_HERMITE,
		KEY_TYPE_HOLD
	};

	struct Keyframe
	{
		int32_t Type = KEY_TYPE_NONE;
		float Frame = 0.0f;
		float Value = 0.0f;
		float T1 = 0.0f, T2 = 0.0f;
	};

	struct Property1D
	{
		int32_t Type = KEY_TYPE_NONE;
		float Value = 0.0f;
		float Max = 0.0f;
		std::vector<Keyframe> Keys;

		inline void AddKey(int32_t type, float f, float v = 0.0f, float t1 = 0.0f, float t2 = 0.0f)
		{
			if (f > Max)
				Max = f;

			Keys.push_back({ type, f, v, t1, t2 });
		}
	};

	struct Property3D
	{
		Property1D X, Y, Z;

		inline float GetMaxFrame() const
		{
			float max = 0.0f;
			if (X.Max > max) max = X.Max;
			if (Y.Max > max) max = Y.Max;
			if (Z.Max > max) max = Z.Max;
			return max;
		}
	};

	struct CameraRoot
	{
		Property3D Translation;
		Property3D Rotation;
		Property3D Scale = SCALE_DEFAULT;
		Property1D Visibility = { 1, 1.0f };

		struct
		{
			float Aspect = 16.0f / 9.0f;
			int32_t FoVIsHorizontal = 0;
			Property1D FoV;
			Property3D Translation;
			Property3D Rotation;
			Property3D Scale = SCALE_DEFAULT;
			Property1D Visibility = { 1, 1.0f };
		} ViewPoint;

		struct
		{
			Property3D Translation;
			Property3D Rotation;
			Property3D Scale = SCALE_DEFAULT;
			Property1D Visibility = { 1, 1.0f };
		} Interest;
	};

	struct ObjectHrc
	{

	};

	class Auth3D
	{
	public:
		int32_t ConverterVersion = 20220912;
		int32_t PropertyVersion = 20050706;
		std::string Filename = "file.a3da";

		std::vector<CameraRoot> Cameras;
		std::vector<ObjectHrc> ObjectHrcs;
		struct
		{
			float Begin = 0.0f;
			float Framerate = 60.0f;
			float Size = 0.0f;
		} PlayControl;

		inline float GetMaxFrame() const
		{
			float max = 0.0f;

			for (const CameraRoot& cam : Cameras)
			{
				float vpMax = cam.ViewPoint.Translation.GetMaxFrame();
				float itMax = cam.Interest.Translation.GetMaxFrame();
				float fovMax = cam.ViewPoint.FoV.Max;

				if (vpMax > max) max = vpMax;
				if (itMax > max) max = itMax;
				if (fovMax > max) max = fovMax;
			}

			return max;
		}

		bool Write(IO::Writer& writer);
	};
}