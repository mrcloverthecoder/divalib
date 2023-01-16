// EditCamera -- Convert EDIT_CAMERA script commands to Auth3D cameras
//

#define _USE_MATH_DEFINES
#include <math.h>
#include <core_io.h>
#include <diva_auth3d.h>

static const char* UsageInfo = "Usage: EditCamera.exe [options] input [output folder]\n"
                               "Options:\n"
							   "\t-p [start part number (e.g. 47)] - Set which PARTS to start at\n"
							   "\t-id [PV id]                      - Set ID for Auth3D name\n"
							   "\t-ns                              - Save as F style Auth3D camera\n";

struct Opcode
{
	int32_t Id = 0;
	int32_t Length = 0; // NOTE: Number of arguments
	const char* Name = nullptr;
};

struct EditCameraInfo
{
	float Duration = 0.0f;
	float ViewPointStartX = 0.0f, ViewPointStartY = 0.0f, ViewPointStartZ = 0.0f;
	float InterestStartX = 0.0f, InterestStartY = 0.0f, InterestStartZ = 0.0f;
	float ViewPointEndX = 0.0f, ViewPointEndY = 0.0f, ViewPointEndZ = 0.0f;
	float InterestEndX = 0.0f, InterestEndY = 0.0f, InterestEndZ = 0.0f;
};

enum OpcodeId : int32_t
{
	OP_END = 0x00,
	OP_TIME = 0x01,
	OP_PV_END = 0x20,
	OP_EDIT_CAMERA = 0x51
};

const Opcode Opcodes[] = {
	{ OP_END, 0, "END" }, { OP_TIME, 1, "TIME" }, { 2, 4, "" }, { 3, 2, "" }, { OP_PV_END, 0, "PV_END" },
	{ OP_EDIT_CAMERA, 24, "EDIT_CAMERA" }
};
const int32_t OpcodeCount = sizeof(Opcodes) / sizeof(Opcode);

constexpr int32_t OpcodeLengthQuick[] = {
	0, 1, 4, 2, 2, 2, 11, 4, 2, 6, 2, 1, 6, 2, 1, 1,
	3, 2, 3, 5, 5, 4, 4, 5, 2, 0, 2, 4, 2, 2, 1, 21,
	0, 3, 2, 5, 1, 1, 7, 1, 1, 2, 1, 2, 1, 2, 3, 3,
	1, 2, 2, 3, 6, 6, 1, 1, 2, 3, 1, 2, 2, 4, 4, 1,
	2, 1, 2, 1, 1, 3, 3, 3, 2, 1, 9, 3, 2, 4, 2, 3,
	2, 24, 1, 2
};

static const Opcode* GetOpcodeInfo(int32_t id)
{
	for (int32_t i = 0; i < OpcodeCount; i++)
		if (Opcodes[i].Id == id)
			return &Opcodes[i];
	return nullptr;
}

static EditCameraInfo FormatEditCamera(int32_t* data)
{
	EditCameraInfo cam = {
		static_cast<float>(data[0]), // MS_DURATION
		data[1] / 1000.0f, data[2] / 1000.0f, data[3] / 1000.0f, // VIEWPOINT_START
		data[4] / 1000.0f, data[5] / 1000.0f, data[6] / 1000.0f, // INTEREST_START
		data[10] / 1000.0f, data[11] / 1000.0f, data[12] / 1000.0f, // VIEWPOINT_END
		data[13] / 1000.0f, data[14] / 1000.0f, data[15] / 1000.0f, // INTEREST_END
	};

	return cam;
}

static bool AddAuthCamRootKey(const EditCameraInfo& camInfo, Auth::CameraRoot& authCam, float timeDelta)
{
	// NOTE: Calculate end frame
	float startFrame = floorf(timeDelta / 1000.0f * 60.0f);
	float endFrame = floorf(startFrame + (camInfo.Duration / 1000.0f * 60.0f));

	// NOTE: Set static FoV value
	authCam.ViewPoint.FoV.Type = Auth::KEY_TYPE_STATIC;
	authCam.ViewPoint.FoV.Value = 65.0f * (static_cast<float>(M_PI) / 180.0f);

	// NOTE: Add start frames
	authCam.ViewPoint.Translation.X.AddKey(Auth::KEY_TYPE_STATIC, startFrame, camInfo.ViewPointStartX);
	authCam.ViewPoint.Translation.Y.AddKey(Auth::KEY_TYPE_STATIC, startFrame, camInfo.ViewPointStartY);
	authCam.ViewPoint.Translation.Z.AddKey(Auth::KEY_TYPE_STATIC, startFrame, camInfo.ViewPointStartZ);
	authCam.Interest.Translation.X.AddKey(Auth::KEY_TYPE_STATIC, startFrame, camInfo.InterestStartX);
	authCam.Interest.Translation.Y.AddKey(Auth::KEY_TYPE_STATIC, startFrame, camInfo.InterestStartY);
	authCam.Interest.Translation.Z.AddKey(Auth::KEY_TYPE_STATIC, startFrame, camInfo.InterestStartZ);

	// NOTE: Add end frames
	authCam.ViewPoint.Translation.X.AddKey(Auth::KEY_TYPE_STATIC, endFrame, camInfo.ViewPointEndX);
	authCam.ViewPoint.Translation.Y.AddKey(Auth::KEY_TYPE_STATIC, endFrame, camInfo.ViewPointEndY);
	authCam.ViewPoint.Translation.Z.AddKey(Auth::KEY_TYPE_STATIC, endFrame, camInfo.ViewPointEndZ);
	authCam.Interest.Translation.X.AddKey(Auth::KEY_TYPE_STATIC, endFrame, camInfo.InterestEndX);
	authCam.Interest.Translation.Y.AddKey(Auth::KEY_TYPE_STATIC, endFrame, camInfo.InterestEndY);
	authCam.Interest.Translation.Z.AddKey(Auth::KEY_TYPE_STATIC, endFrame, camInfo.InterestEndZ);

	// NOTE: Set key types
	authCam.ViewPoint.Translation.X.Type = Auth::KEY_TYPE_LINEAR;
	authCam.ViewPoint.Translation.Y.Type = Auth::KEY_TYPE_LINEAR;
	authCam.ViewPoint.Translation.Z.Type = Auth::KEY_TYPE_LINEAR;
	authCam.Interest.Translation.X.Type = Auth::KEY_TYPE_LINEAR;
	authCam.Interest.Translation.Y.Type = Auth::KEY_TYPE_LINEAR;
	authCam.Interest.Translation.Z.Type = Auth::KEY_TYPE_LINEAR;

	return true;
}

static void OptimizeKeyStepPop(Auth::Property1D& prop)
{
	if (prop.Keys.size() < 2)
		return;

	size_t indexLast = prop.Keys.size() - 1;
	
	while (prop.Keys.size() > 1 && (prop.Keys[indexLast].Value == prop.Keys[indexLast - 1].Value))
	{
		prop.Keys.pop_back();
		indexLast--;
	}
}

static void OptimizeKeyStepMakeStatic(Auth::Property1D& prop)
{
	if (prop.Keys.size() != 1)
		return;

	prop.Type = Auth::KEY_TYPE_STATIC;
	prop.Value = prop.Keys[0].Value;
}

static void OptimizeProperty3D(Auth::Property3D& prop)
{
	OptimizeKeyStepPop(prop.X);
	OptimizeKeyStepMakeStatic(prop.X);
	OptimizeKeyStepPop(prop.Y);
	OptimizeKeyStepMakeStatic(prop.Y);
	OptimizeKeyStepPop(prop.Z);
	OptimizeKeyStepMakeStatic(prop.Z);
}

static void OptimizeKeyframes(Auth::CameraRoot& cam)
{
	OptimizeProperty3D(cam.Interest.Translation);
	OptimizeProperty3D(cam.ViewPoint.Translation);
}

static void WriteAuthSingleShot(const EditCameraInfo& camInfo, int32_t camIndex, int32_t id)
{
	Auth::Auth3D a3d = { };
	IO::Writer writer;

	// NOTE: Format filename
	char buffer[0x100] = { '\0' };
	if (camIndex < 1)
		sprintf_s(buffer, 0x100, "CAMPV%03d_BASE.a3da", id);
	else
		sprintf_s(buffer, 0x100, "CAMPV%03d_PARTS%02d.a3da", id, camIndex + 1);

	a3d.Filename = std::string(buffer);
	AddAuthCamRootKey(camInfo, a3d.Cameras.emplace_back(), 0.0f);
	OptimizeKeyframes(a3d.Cameras[0]);

	a3d.PlayControl.Begin = 0.0f;
	a3d.PlayControl.Framerate = 60.0f;
	a3d.PlayControl.Size = a3d.GetMaxFrame();

	a3d.Write(writer);
	writer.Flush(buffer);
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		puts(UsageInfo);
		return -1;
	}

	const char *inputFilename = nullptr, *outPath = ".\\";
	int32_t id = 0;
	int32_t camIndex = 0;
	bool authStyleF = false;

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-id") == 0)
			id = strtol(argv[++i], nullptr, 10);
		else if (strcmp(argv[i], "-p") == 0)
			camIndex = strtol(argv[++i], nullptr, 10);
		else if (strcmp(argv[i], "-ns") == 0)
			authStyleF = true;
		else if (inputFilename == nullptr)
			inputFilename = argv[i];
		else if (outPath == nullptr)
			outPath = argv[i];
	};

	if (inputFilename == nullptr)
		return -1;

	if (camIndex > 0)
		camIndex--;

	// NOTE: Open DSC reader
	IO::Reader reader;
	reader.FromFile(inputFilename);
	reader.SeekBegin(0x04);

	Auth::Auth3D a3d = { };
	auto& camRoot = a3d.Cameras.emplace_back();

	int32_t time = 0;
	int32_t timeBegin = -1;
	int32_t op = 0;
	int32_t argBuffer[144] = { 0 };
	do
	{
		op = reader.ReadInt32();
		const Opcode* opInfo = GetOpcodeInfo(op);
		/*if (opInfo == nullptr)
		{
			printf("Unknown opcode: %d\n", op);
			return -1;
		}*/

		if (opInfo != nullptr)
			reader.Read(argBuffer, opInfo->Length * sizeof(int32_t));

		switch (op)
		{
			case OP_END:
				break;
			case OP_TIME:
				time = argBuffer[0];
				break;
			case OP_EDIT_CAMERA:
			{
				// NOTE: Keep track of the timestamp of the first EDIT_CAMERA command
				if (timeBegin < 0)
					timeBegin = time;

				// NOTE: Properly format DSC EDIT_CAMERA data into a readable struct
				const auto camInfo = FormatEditCamera(argBuffer);

				if (!authStyleF)
					WriteAuthSingleShot(camInfo, camIndex, id);
				else
					AddAuthCamRootKey(camInfo, camRoot, (time - timeBegin) / 100.0f);

				camIndex++;
				break;
			}
			default:
				reader.SeekCurrent(OpcodeLengthQuick[op] * 0x04);
				break;
		}
	} while (op != 0);

	if (authStyleF)
	{
		OptimizeKeyframes(camRoot);
		a3d.PlayControl = { 0.0f, 60.0f, a3d.GetMaxFrame() };

		// NOTE: Format filename
		char filename[0x100] = { '\0' };
		sprintf_s(filename, 0x100, "CAMPV%03d_BASE.a3da", id);
		// ...and set Auth3D internal filename
		a3d.Filename = filename;

		// NOTE: Write Auth3D to file
		IO::Writer writer;
		a3d.Write(writer);
		writer.Flush(filename);
	}

	return 0;
}