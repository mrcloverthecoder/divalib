#include "pch.h"
#include "diva_archive.h"

using namespace Archive;

bool Archive::FArc::Open(std::string_view path)
{
	mStream.FromFile(path);
	mStream.SetEndianness(IO::Endianness::Big);
	ReadHeader();

	return true;
}

void Archive::FArc::Close()
{
	mFiles.clear();
}

Archive::FArc::FileData Archive::FArc::GetFile(std::string_view name)
{
	FileData file = { };

	for (const FileEntry& entry : mFiles)
	{
		if (strcmp(name.data(), entry.Name.data()) != 0)
			continue;

		file.Size = entry.Size;
		file.Data = new uint8_t[entry.Size];

		// Try reading the data from the archive
		mStream.SeekBegin(entry.Offset);
		if (!mIsCompressed)
			mStream.Read(file.Data, file.Size);
		else
		{

		}
	}

	return file;
}

void Archive::FArc::ReadHeader()
{
	int32_t signature = mStream.ReadInt32();
	int32_t headerSize = mStream.ReadInt32();
	int32_t alignment = mStream.ReadInt32();

	switch (signature)
	{
	case 'FArc':
		mIsCompressed = false;
		mIsEncrypted  = false;
		break;
	case 'FArC':
		mIsCompressed = true;
		mIsEncrypted  = false;
		break;
	case 'FARC':
		mIsCompressed = true;
		mIsEncrypted  = true;
		break;
	}

	// Read file entries
	while (mStream.GetPosition() < (size_t)(headerSize + 8))
	{
		FileEntry entry = { };
		entry.Name = mStream.ReadString(); // File name
		entry.Offset = mStream.ReadInt32();
		if (mIsCompressed)
			entry.CompressedSize = mStream.ReadInt32();
		entry.Size = mStream.ReadInt32();

		mFiles.push_back(entry);
	}
}

constexpr size_t FArcAlignment = 0x10;

bool FArcPacker::AddFile(const FArcPacker::FArcFile& file)
{
	if (file.Filename.empty() || !file.Data || file.Size < 1)
		return false;

	mFiles.push_back(file);
	return true;
}

bool FArcPacker::Flush(std::string_view path, bool compress)
{
	if (path.empty())
		return false;
	
	IO::Writer writer;
	writer.SetEndianness(IO::Endianness::Big);

	int32_t headerSize = GetHeaderSize(compress);
	int32_t fileOffset = headerSize + 0x08;

	writer.WriteInt32('FArc');
	writer.WriteInt32(headerSize);
	writer.WriteInt32(FArcAlignment);

	// Write header
	for (const FArcFile& file : mFiles)
	{
		writer.WriteString(file.Filename);
		writer.WriteInt32(fileOffset);
		writer.WriteInt32(file.Size);

		fileOffset += file.Size;
		fileOffset = IO::Util::Align(fileOffset, FArcAlignment);
	}

	// Alin header
	writer.Pad(FArcAlignment, 'x');

	// Write file data
	for (const FArcFile& file : mFiles)
	{
		writer.Write(file.Data, file.Size);
		writer.Pad(FArcAlignment, 'x');
	}

	writer.Flush(path);

	return true;
}

int32_t FArcPacker::GetHeaderSize(bool compressed)
{
	int32_t size = 0x04; // Alignment value

	for (const FArcFile& file : mFiles)
	{
		size += file.Filename.size() + 1;
		size += 0x08; // Offset and size
		if (compressed)
			size += 0x04; // Compressed size
	}

	return size;
}
