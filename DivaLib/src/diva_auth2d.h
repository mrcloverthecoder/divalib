#pragma once

#include <string>
#include <vector>
#include "io_core.h"

namespace Aet
{
	enum class Quality : uint8_t
	{
		None = 0,
		Wireframe,
		Draft,
		Best
	};

	enum class ItemType : uint8_t
	{
		None = 0,
		Video,
		Audio,
		Composition
	};

	struct LayerFlags
	{
		bool VideoActive : 1;
		bool AudioActive : 1;
		bool EffectActive : 1;
		bool MotionBlur : 1;
		bool FrameBlending : 1;
		bool Locked : 1;
		bool Shy : 1;
		bool Collapse : 1;
		bool AutoOrientRotation : 1;
		bool AdjustmentLayer : 1;
		bool TimeRemapping;
		bool LayerIs3D : 1;
		bool LookAtCamera : 1;
		bool LookAtPointOfInterest : 1;
		bool Solo : 1;
		bool MarkersLocked : 1;
	};

	enum class BlendMode
	{
		None = 0,
		Copy,
		Behind,
		Normal,
		Dissolve,
		Add,
		Multiply,
		Screen,
		Overlay,
		SoftLight,
		HardLight,
		Darken,
		Lighten,
		ClassicDifference,
		Hue,
		Saturation,
		Color,
		Luminosity,
		StencilAlpha,
		StencilLuma,
		SilhouetteAlpha,
		SilhouetteLuma,
		LuminescentPremultiplied,
		AlphaAdd,
		ClassicColorDodge,
		ClassicColorBurn,
		Exclusion,
		Difference,
		ColorDodge,
		ColorBurn,
		LinearDodge,
		LinearBurn,
		LinearLight,
		VividLight,
		PinLight,
		HardMix,
		LighterColor,
		DarkerColor,
		Subtract,
		Divide
	};

	enum class TrackMatteMode
	{
		None = 0,
		Alpha,
		NotAlpha,
		Luma,
		NotLuma
	};

	struct Keyframe1D
	{
		float Frame;
		float Value;
		float Tangent;

		Keyframe1D() = default;
		Keyframe1D(float f, float v, float t) : Frame(f), Value(v), Tangent(t) { }
	};

	struct Property1D
	{
		std::vector<Keyframe1D> Keyframes;
	};

	struct LayerVideo
	{
		struct TransferModeFlags
		{
			bool PreserveAlpha : 1;
			bool RandomizeDissolve : 1;
			bool : 0;
		};

		struct TransferMode
		{
			BlendMode BlendMode;
			TrackMatteMode TrackMatte;
			TransferModeFlags Flags;
			uint8_t Reserved;
		} TransferMode = { BlendMode::Normal, TrackMatteMode::None, 0, 0 };
		// TODO: Change these to Property2D (need to implement vectors first)
		Property1D AnchorX, AnchorY;
		Property1D PositionX, PositionY;
		Property1D Rotation;
		Property1D ScaleX, ScaleY;
		Property1D Opacity;
	};

	struct LayerAudio
	{

	};

	struct Layer
	{
		std::string Name;
		float StartTime;
		float EndTime;
		float OffsetTime;
		float TimeScale;
		LayerFlags Flags;
		Quality Quality;
		ItemType ItemType;
		void* Item;
		LayerVideo Video;
		LayerAudio Audio;
	};

	struct Composition
	{
		std::vector<Layer> Layers;
	};

	struct VideoSrc
	{
		std::string Name;
		uint32_t Id;
	};

	struct Video
	{
		uint8_t Color[4];
		uint16_t Width, Height;
		float Frames;

		std::vector<VideoSrc> Sources;
	};

	struct Scene
	{
		std::string Name;
		float StartFrame, EndFrame;
		float Framerate;
		uint8_t BackgroundColor[4];
		int32_t Width, Height;

		std::vector<Composition> Compositions;
		std::vector<Video> Videos;
	};

	class AetSet
	{
	public:
		std::vector<Scene> Scenes;

		void Parse(IO::Stream& stream);
	};
}