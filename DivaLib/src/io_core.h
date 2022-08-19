#pragma once

#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <string_view>

namespace IO
{
	namespace Util
	{
		size_t Align(size_t value, size_t alignment);
	}

	namespace Endian
	{
		int32_t Swap(int32_t value);
	}

	namespace Path
	{
		std::string GetFilename(std::string_view path);
	}

	enum class StreamMode
	{
		Read,
		Write,
		Append
	};

	enum class EndiannessMode
	{
		Little,
		Big
	};

	class Stream
	{
	public:
		virtual bool Read(void* buffer, size_t size) = 0;
		virtual bool Write(const void* buffer, size_t size) = 0;

		float ReadFloat32();
		uint8_t ReadUInt8();
		uint16_t ReadUInt16();
		int32_t ReadInt32();
		uint32_t ReadUInt32();
		void ReadNullString(char* buffer, size_t bufferSize);
		void ReadNullString(std::string& str);
		void ReadNullStringOffset(std::string& str);
		// Reads offset (uint32_t) and calls ExecuteAtOffset(offset, task)
		void ExecuteOffset(std::function<void(void)> task);
		// Execute task at offset
		void ExecuteAtOffset(uint32_t offset, std::function<void(void)> task);

		bool Align(size_t alignment, char padding = '\0');
		bool WriteNull(size_t count);
		bool WriteInt32(int32_t value);
		bool WriteNullString(const std::string& str);

		inline void SetEndian(EndiannessMode mode) { Endianness = mode; }
		inline EndiannessMode GetEndian() { return Endianness; }
		
		virtual bool Valid() = 0;
		virtual bool Close() = 0;
		virtual uint32_t Tell() = 0;
		virtual void SeekBegin(uint32_t offset) = 0;
		virtual void SeekCurrent(uint32_t offset) = 0;
	protected:
		EndiannessMode Endianness = EndiannessMode::Little;

		Stream() { }
	};

	class File : public Stream
	{
	public:
		File() : mDiskHandle(nullptr)
		{
			SetEndian(EndiannessMode::Little);
		}

		// Open a file object from disk
		static File Open(std::string_view path, StreamMode mode);

		/*
		template <typename T>
		T Load(std::string_view path);
		*/

		// Read data from disk
		bool Read(void* buffer, size_t size) override;
		// Write data to disk
		bool Write(const void* buffer, size_t size) override;
		// Checks if the file handle is valid
		inline bool Valid() override { return mDiskHandle != nullptr; }
		// Release file handle
		bool Close() override;
		// Get position of read head
		uint32_t Tell() override;
		// Set position of read head
		void SeekBegin(uint32_t offset) override;
		void SeekCurrent(uint32_t offset) override;
	private:
		FILE* mDiskHandle;
	};
}