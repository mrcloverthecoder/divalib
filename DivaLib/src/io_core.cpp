#include "pch.h"
#include "io_core.h"
#include "util_string.h"

// C (fopen_s) file mode table
static const char* CFileMode[3] = { "rb", "wb", "ab" };

// IO utilities
size_t IO::Util::Align(size_t value, size_t alignment)
{
	while (value % alignment != 0)
		value += 1;

	return value;
}

// Endianness helpers
int32_t IO::Endian::Swap(int32_t value)
{
	return (value >> 24 & 0xFF) | ((value >> 16 & 0xFF) << 8) | ((value >> 8 & 0xFF) << 16) | ((value & 0xFF) << 24);
}

// Path manipulation
std::string IO::Path::GetFilename(std::string_view path)
{
	std::string filename;

	// Try to find index of path separators
	int32_t idxU = ::Util::String::GetLastIndex(path, '/');  // Unix
	int32_t idxW = ::Util::String::GetLastIndex(path, '\\'); // Windows
	int32_t idx = (idxU > idxW ? idxU : idxW) + 1;

	if (idx > 0)
		for (size_t i = idx; i < path.size(); i++)
			filename += path.at(i);
	else
		filename = { path.begin(), path.end() }; // Copy data

	return filename;
}



// Stream
float IO::Stream::ReadFloat32()
{
	float value = 0;
	Read(&value, sizeof(float));

	return value;
}

// TODO: Use template for integer reading functions?
uint8_t IO::Stream::ReadUInt8()
{
	uint8_t value = 0;
	Read(&value, sizeof(uint8_t));
	return value;
}

uint16_t IO::Stream::ReadUInt16()
{
	uint16_t value = 0;
	Read(&value, sizeof(uint16_t));
	return value;
}

int32_t IO::Stream::ReadInt32()
{
	int32_t value = 0;
	Read(&value, sizeof(int32_t));

	if (Endianness == EndiannessMode::Big)
		return Endian::Swap(value);

	return value;
}

uint32_t IO::Stream::ReadUInt32()
{
	uint32_t value = 0;
	Read(&value, sizeof(uint32_t));

	if (Endianness == EndiannessMode::Big)
		return (uint32_t)Endian::Swap(value);

	return value;
}

// There must be a better way to do this
void IO::Stream::ReadNullString(char* buffer, size_t bufferSize)
{
	// Size of the string
	uint32_t size = 0;
	// Pointer to the current position in buffer
	char* head = buffer;
	// Set current char to 0x01 initially to
	// make sure the loop will run correctly
	char current = 0x01;

	while (size < bufferSize && current != '\0')
	{
		// Read single char
		Read(&current, sizeof(char));
		// Copy it to the buffer
		*head = current;
		// Advance buffer and size;
		head++;
		size += sizeof(char);
	}
}

// It is more optimized to push data directly into
// the destination string instead of returning one
// (latter case would lead to unnecessary copying)
void IO::Stream::ReadNullString(std::string& str)
{
	char current = 0x7F;
	do
	{
		Read(&current, sizeof(char));
		str += current;
	} while (current != '\0');
}

void IO::Stream::ReadNullStringOffset(std::string& str)
{
	// Read offset value and get read head position
	uint32_t offset = ReadUInt32();
	uint32_t curPos = Tell();
	// Seek to the string's offset and read the string
	SeekBegin(offset);
	ReadNullString(str);
	// Seek back to read head position
	SeekBegin(curPos);
}

void IO::Stream::ExecuteOffset(std::function<void(void)> task)
{
	uint32_t offset = ReadUInt32();
	// ExecuteAtOffset handles seeking back already
	ExecuteAtOffset(offset, task);
}

void IO::Stream::ExecuteAtOffset(uint32_t offset, std::function<void(void)> task)
{
	if (offset == 0)
		return;

	// Get current position
	uint32_t pos = Tell();

	// Execute work at offset
	SeekBegin(offset);
	task();

	// Seek back to current position;
	SeekBegin(pos);
}

bool IO::Stream::Align(size_t alignment, char padding)
{
	while (Tell() % alignment != 0)
		Write(&padding, 1);

	return true;
}

bool IO::Stream::WriteNull(size_t count)
{
	if (count < 1)
		return false;

	int null = 0;
	for (size_t i = 0; i < count; i++)
		if (!Write(&null, 1))
			return false;

	return true;
}

bool IO::Stream::WriteInt32(int32_t value)
{
	if (Endianness == EndiannessMode::Big)
		value = IO::Endian::Swap(value);

	return Write(&value, sizeof(int32_t));
}

bool IO::Stream::WriteNullString(const std::string& str)
{
	if (str.size() < 1)
		return false;

	Write(str.data(), str.size());
	WriteNull(1);

	return true;
}

// File handling
IO::File IO::File::Open(std::string_view path, IO::StreamMode mode)
{
	IO::File file;

	// Return early is path is empty
	if (path.empty())
		return file;

	// Try open file from disk
	FILE* stream;
	errno_t status = fopen_s(&stream, path.data(), CFileMode[static_cast<int>(mode)]);

	// Only set the handle if the file opened properly
	if (status == 0)
		file.mDiskHandle = stream;

	return file;
}

bool IO::File::Read(void* buffer, size_t size)
{
	fread_s(buffer, size, size, 1, mDiskHandle);
	return true;
}

bool IO::File::Write(const void* buffer, size_t size)
{
	size_t num = fwrite(buffer, size, 1, mDiskHandle);
	// Number of bytes written is different than the one requested?
	if (num != size)
		return false;

	return true;
}

bool IO::File::Close()
{
	if (mDiskHandle)
	{
		fclose(mDiskHandle);
		mDiskHandle = nullptr;
	}

	return true;
}

uint32_t IO::File::Tell()
{
	if (mDiskHandle)
		return ftell(mDiskHandle);
	return 0;
}

void IO::File::SeekBegin(uint32_t offset)
{
	if (mDiskHandle)
		fseek(mDiskHandle, offset, SEEK_SET);
}

void IO::File::SeekCurrent(uint32_t offset)
{
	if (mDiskHandle)
		fseek(mDiskHandle, offset, SEEK_CUR);
}