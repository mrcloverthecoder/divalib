#include "pch.h"
#include "core_io.h"
#include "util_string.h"

// IO utilities
size_t IO::Util::Align(size_t value, size_t alignment)
{
	while (value % alignment != 0)
		value += 1;

	return value;
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

struct SegmentInfo
{
	size_t Position = 0;
	size_t Size = 0;
};

static bool GetSegmentInfo(std::string_view path, std::vector<SegmentInfo>* outSegInfo, size_t* outCount)
{
	if (path.empty())
		return false;

	int32_t count = 1;
	size_t lastPosition = 0;
	size_t lastPositionEnd = 0;
	for (size_t idx = 0; idx < path.size(); idx++)
	{
		// NOTE: Check if the character is a path separator and if its not the
		//       first character in the path (UNIX absolute paths)
		if (IO::Path::IsPathSeparator(path.at(idx)) && idx > 0)
		{
			// NOTE: Check if this isn't the last character in the path and
			//       then check if the next character isn't a path separator
			if (idx + 1 < path.size() && !IO::Path::IsPathSeparator(path.at(idx + 1)))
			{
				// NOTE: If all these checks are successful, store the
				//       segment data
				if (outSegInfo != nullptr)
					outSegInfo->push_back({ lastPosition, lastPositionEnd - lastPosition + 1 });
				lastPosition = idx + 1;
				count++;
			}
		}

		if (!IO::Path::IsPathSeparator(path.at(idx)))
			lastPositionEnd = idx;

		// NOTE: Add the last part of the path to the segments list
		//       if we've reached the last character in the path
		if (idx == path.size() - 1)
			if (outSegInfo != nullptr)
				outSegInfo->push_back({ lastPosition, lastPositionEnd - lastPosition + 1 });
	}

	if (outCount != nullptr)
		*outCount = count;
	return true;
}

size_t IO::Path::GetSegmentCount(std::string_view path)
{
	size_t count = 0;
	GetSegmentInfo(path, nullptr, &count);
	return count;
}

static std::string GetSegmentRange(std::string_view path, size_t startIndex, size_t endIndex)
{
	if (path.empty())
		return std::string();

	std::vector<SegmentInfo> segsInfo;
	size_t count = 0;
	GetSegmentInfo(path, &segsInfo, &count);

	if (startIndex >= count || endIndex >= count)
		return std::string();

	if (startIndex == endIndex)
		return std::string(&path[segsInfo[startIndex].Position], segsInfo[startIndex].Size);

	std::string pathRanged;
	for (size_t idx = startIndex; idx <= endIndex; idx++)
	{
		pathRanged += std::string(&path[segsInfo[idx].Position], segsInfo[idx].Size);
		if (idx < endIndex)
			pathRanged += "/";
	}

	return pathRanged;
}

std::string IO::Path::GetSegment(std::string_view path, int32_t segmentIndex)
{
	return GetSegmentRange(path, segmentIndex, segmentIndex);
}

std::string IO::Path::GetUntilSegment(std::string_view path, int32_t lastSegIndex)
{
	return GetSegmentRange(path, 0, lastSegIndex);
}

namespace Helper
{
	static inline size_t GetPosition(FILE* file) { return (size_t)ftell(file); }
	static inline void SetPosition(FILE* file, size_t pos) { fseek(file, pos, SEEK_SET); }

	static size_t GetFileSize(FILE* file)
	{
		size_t position = GetPosition(file);

		fseek(file, 0, SEEK_END);
		size_t size = GetPosition(file);
		SetPosition(file, position);

		return size;
	}
}

// TODO: Make a function to narrow std::string_view to null-terminated std::string
IO::FileBuffer IO::File::ReadAllData(std::string_view path, bool nullTerminated)
{
	// Attempt to open file
	FILE* handle = nullptr;
	fopen_s(&handle, path.data(), "rb");

	// Attempt to read all of it's data
	FileBuffer buffer = { };

	if (handle != nullptr)
	{
		// Retrieve file size
		size_t fileSize = Helper::GetFileSize(handle);
		size_t size = nullTerminated ? fileSize + 1 : fileSize;
		// Create buffer
		buffer.Content = std::make_unique<uint8_t[]>(size);
		buffer.Size = fileSize;
		// Read all file data into the buffer
		fread(buffer.Content.get(), fileSize, 1, handle);
		if (nullTerminated)
			buffer.Content[fileSize] = 0x00;
		// Close disk file
		fclose(handle);
	}

	return buffer;
}

bool IO::File::Exists(std::string_view path)
{
	FILE* handle = nullptr;
	if (fopen_s(&handle, path.data(), "rb") == 0)
	{
		fclose(handle);
		return true;
	}

	return false;
}

bool IO::Directory::Create(std::string_view path)
{
	if (path.empty())
		return false;

	size_t segCount = Path::GetSegmentCount(path);
	for (size_t i = 0; i < segCount; i++)
	{
		std::string curDir = Path::GetUntilSegment(path, i);
		if (Path::IsDrive(curDir) || Directory::Exists(curDir))
			continue;

		if (CreateDirectoryA(curDir.c_str(), nullptr) == 0)
			return false;
	}

	return true;
}

bool IO::Directory::Exists(std::string_view path)
{
	DWORD attrib = GetFileAttributesA(path.data());
	if (attrib != INVALID_FILE_ATTRIBUTES)
		return (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
	return false;
}

void IO::MemoryReader::FromFile(std::string_view path)
{
	FileBuffer buffer = File::ReadAllData(path);

	// Initialize
	if (buffer.Content.get() != nullptr)
	{
		// Move read content to this instance
		mContent = std::move(buffer.Content);
		// Initialize other variables
		mSize = buffer.Size;
		mPosition = 0;
	}
	else
		printf("[MemoryReader::OpenFile] File (%s) does not exist.\n", path.data());
}

bool IO::MemoryReader::Read(void* buffer, size_t size)
{
	if (mPosition + size > mSize)
		return false;

	// Copy source data to the destination buffer
	memcpy(buffer, &mContent[mPosition], size);
	// Advance read head position
	mPosition += size;
	return true;
}

std::string IO::MemoryReader::ReadString()
{
	std::string str;
	char c = '\0';
	do
	{
		c = ReadChar();
		str += c;
	} while (c != '\0');

	return str;
}

void IO::MemoryReader::ExecuteAtOffset(uint32_t offset, std::function<void(void)> task)
{
	if (offset == 0)
		return;

	// Get current position
	uint32_t pos = GetPosition();

	// Execute work at offset
	SeekBegin(offset);
	task();

	// Seek back to current position;
	SeekBegin(pos);
}

bool IO::MemoryWriter::Write(const void* buffer, size_t size)
{
	if (size < 1 || buffer == nullptr)
		return false;

	while (mPosition + size > mCapacity)
		ExpandBuffer();

	memcpy(&mContent[mPosition], buffer, size);
	if (mPosition + size > mSize)
		mSize += mPosition + size - mSize;
	mPosition += size;

	return true;
}

void IO::MemoryWriter::ScheduleWrite(std::function<void(IO::MemoryWriter&)> task)
{
	auto& schedule = ScheduledWrites.emplace_back();
	schedule.Task = task;
}

void IO::MemoryWriter::ScheduleWriteOffset(std::function<void(IO::MemoryWriter&)> task)
{
	auto& schedule = ScheduledWrites.emplace_back();
	schedule.Task = task;
	schedule.OffsetPosition = GetPosition();
	WriteUInt32(0);
}

void IO::MemoryWriter::ScheduleWriteOffsetAndSize(std::function<void(IO::MemoryWriter&)> task)
{
	auto& schedule = ScheduledWrites.emplace_back();
	schedule.Task = task;
	schedule.OffsetPosition = GetPosition();
	WriteUInt32(0);
	schedule.SizePosition = GetPosition();
	WriteUInt32(0);
}

void IO::MemoryWriter::ScheduleWriteStringOffset(std::string& data)
{
	auto& schedule = ScheduledStrings.emplace_back();
	schedule.Data = data;
	schedule.OffsetPosition = GetPosition();
	WriteUInt32(0);
}

void IO::MemoryWriter::FlushScheduledWrites()
{
	// NOTE: Make sure we're at the end of the file
	SeekEnd(0);

	for (auto& schedule : ScheduledWrites)
	{
		// NOTE: Write data, get offset and size
		size_t pos = GetPosition();
		schedule.Task(*this);
		size_t size = GetPosition() - pos;

		// NOTE: Write offset and size
		if (schedule.OffsetPosition > -1)
		{
			Seek(schedule.OffsetPosition);
			WriteUInt32(pos);
		}

		if (schedule.SizePosition > -1)
		{
			Seek(schedule.SizePosition);
			WriteUInt32(size);
		}
		
		// NOTE: Go back to the end of the file
		SeekEnd(0);
	}
}

void IO::MemoryWriter::FlushScheduledStrings()
{
	SeekEnd(0);

	for (auto& schedule : ScheduledStrings)
	{
		if (schedule.OffsetPosition < 0)
			continue;

		size_t pos = GetPosition();
		WriteString(schedule.Data);
		Seek(schedule.OffsetPosition);
		WriteUInt32(pos);
		SeekEnd(0);
	}
}

bool IO::MemoryWriter::Flush(std::string_view path)
{
	FILE* file = nullptr;
	fopen_s(&file, path.data(), "wb");

	if (file == nullptr)
		return false;

	fwrite(mContent.get(), mSize, 1, file);
	fclose(file);

	return true;
}

bool IO::MemoryWriter::CopyTo(IO::MemoryWriter& destination)
{
	return destination.Write(mContent.get(), mSize);
}