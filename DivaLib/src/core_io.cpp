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
IO::FileBuffer IO::File::ReadAllData(std::string_view path)
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
		// Create buffer
		buffer.Content = std::make_unique<uint8_t[]>(fileSize);
		buffer.Size = fileSize;
		// Read all file data into the buffer
		fread(buffer.Content.get(), fileSize, 1, handle);
		// Close disk file
		fclose(handle);
	}

	return buffer;
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
	mSize += size;
	mPosition += size;

	return true;
}

bool IO::MemoryWriter::Flush(std::string_view path)
{
	FILE* file = nullptr;
	fopen_s(&file, path.data(), "wb");

	if (file == nullptr)
		return false;

	fwrite(mContent.get(), mSize, 1, file);
	fclose(file);
}