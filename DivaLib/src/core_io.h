#pragma once

#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <memory>
#include <string_view>
#include "core.h"

namespace IO
{
	namespace Util
	{
		size_t Align(size_t value, size_t alignment);
	}

	namespace Endian
	{
		inline int32_t Swap(int32_t value)
		{
			return (value >> 24 & 0xFF) | ((value >> 16 & 0xFF) << 8) | ((value >> 8 & 0xFF) << 16) | ((value & 0xFF) << 24);
		}

		inline uint32_t Swap(uint32_t value) { (uint32_t)Swap((int32_t)value); }
	}

	namespace Path
	{
		std::string GetFilename(std::string_view path);
	}

	enum class Endianness
	{
		Little,
		Big
	};

	struct FileBuffer
	{
		std::unique_ptr<uint8_t[]> Content;
		size_t Size;
	};

	namespace File
	{
		FileBuffer ReadAllData(std::string_view path);
	}

	// WARNING: Do *not* use this with big files (such as whole data archives).
	//          This is a memory stream, not a file stream. It will load all the data in memory
	class MemoryReader final : NonCopyable
	{
	public:
		MemoryReader() = default;
		~MemoryReader() = default;

		void FromFile(std::string_view path);

		inline size_t GetPosition() { return mPosition; }
		inline size_t GetSize() { return mSize; }
		inline void SetEndianness(Endianness endian) { mEndianness = endian; }

		// TODO: Add more error handling
		inline void SeekBegin(size_t pos) { mPosition = pos; }
		inline void SeekCurrent(size_t offset) { mPosition += offset; }
		inline void SeekEnd(size_t offset) { mPosition = mSize - offset; }
		inline void Align(size_t alignment)
		{
			while (GetPosition() % alignment != 0)
				SeekCurrent(0x01);
		}

		bool Read(void* buffer, size_t size);
		inline char ReadChar() { return Internal_ReadT<char>(); }
		inline int8_t ReadInt8() { return Internal_ReadT<int8_t>(); }
		inline uint8_t ReadUInt8() { return Internal_ReadT<uint8_t>(); }
		inline int16_t ReadInt16() { return Internal_ReadT<int16_t>(); }
		inline uint16_t ReadUInt16() { return Internal_ReadT<uint16_t>(); }
		inline int32_t ReadInt32() { return Internal_ReadT<int32_t>(); }
		inline uint32_t ReadUInt32() { return Internal_ReadT<uint32_t>(); }
		inline float ReadFloat32() { return Internal_ReadT<float>(); }
		// Read null-terminated string
		inline std::string ReadString()
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

		inline std::string ReadStringOffset()
		{
			std::string str;
			uint32_t offset = ReadUInt32();
			ExecuteAtOffset(offset, [&str, this] { str = ReadString(); });
			return str;
		}

		// Execute task at offset
		void ExecuteAtOffset(uint32_t offset, std::function<void(void)> task);
	private:
		std::unique_ptr<uint8_t[]> mContent;
		size_t mSize = 0;
		size_t mPosition = 0;
		Endianness mEndianness = Endianness::Little;

		// Basic types only
		template <typename T>
		inline T Internal_ReadT()
		{
			T value;
			Read(&value, sizeof(T));
			return value;
		}
	};

	class MemoryWriter : NonCopyable
	{
	public:
		MemoryWriter()
		{
			mContent = std::make_unique<uint8_t[]>(mBufferSize);
			mSize = 0;
			mPosition = 0;
			mCapacity = mBufferSize;
			mEndianness = Endianness::Little;
		}
		~MemoryWriter() = default;

		inline void SetEndianness(Endianness endian) { mEndianness = endian; }

		bool Write(const void* buffer, size_t size);
		inline void WriteChar(char value) { Write(&value, sizeof(char)); }
		// I know this is very hacky. I'm just really lazy, LOL
		inline void WriteInt32(int32_t value) { ShouldSwap() ? Write32B(value) : Write32L(value); }
		inline void WriteUInt32(uint32_t value) { WriteInt32(*(int32_t*)&value); }
		inline void WriteFloat32(float value) { WriteInt32(*(int32_t*)&value); }
		inline void WriteString(std::string_view value)
		{
			for (const char& c : value)
				WriteChar(c);
			WriteChar('\0');
		}

		inline void Pad(size_t alignment, char padding = '\0')
		{
			while (mPosition % alignment != 0)
				WriteChar(padding);
		}

		bool Flush(std::string_view path);
	private:
		std::unique_ptr<uint8_t[]> mContent;
		size_t mSize, mPosition, mCapacity;
		Endianness mEndianness;
		const size_t mBufferSize = 256 * 1024; // 256kb

		inline bool ShouldSwap() { return mEndianness == Endianness::Big; }
		inline void Write32L(int32_t value) { Write(&value, sizeof(int32_t)); }
		inline void Write32B(int32_t value) { int32_t v = Endian::Swap(value); Write(&v, sizeof(int32_t)); }

		inline void ExpandBuffer()
		{
			auto cont = std::make_unique<uint8_t[]>(mCapacity + mBufferSize);
			if (cont == nullptr)
				return;

			memcpy(cont.get(), mContent.get(), mSize);
			mContent = std::move(cont);
			mCapacity += mBufferSize;
		}
	};

	// NOTE: Aliases for easier typing
	using Reader = MemoryReader;
	using Writer = MemoryWriter;
}