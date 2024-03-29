#pragma once

#include <string>
#include <string_view>
#include <vector>
#include "core_io.h"

namespace Archive
{
	class FArc
	{
	public:
		struct FileData
		{
			int32_t Size;
			void* Data;

			inline bool Valid() { return Data != nullptr; }
			inline void Close()
			{
				if (Data != nullptr)
					delete[] Data;
				Data = nullptr;
			}
		};

		FArc() : mIsCompressed(false), mIsEncrypted(false) { }

		bool Open(std::string_view path);
		void Close();
		FileData GetFile(std::string_view name);
	private:
		struct FileEntry
		{
			std::string Name;
			int32_t Offset;
			int32_t Size;
			int32_t CompressedSize;
		};

		std::vector<FileEntry> mFiles;
		IO::Reader mStream;

		// Archive flags
		bool mIsCompressed; // FArC, FARC
		bool mIsEncrypted;  // FARC

		void ReadHeader();
	};

	class FArcPacker
	{
	public:
		struct FArcFile
		{
			std::string Filename;
			size_t Size;
			const void* Data;
		};

		FArcPacker() { }

		bool AddFile(const FArcFile& file);
		bool Flush(std::string_view path, bool compress);
	private:
		std::vector<FArcFile> mFiles;

		size_t GetHeaderSize(bool compressed);
	};
}