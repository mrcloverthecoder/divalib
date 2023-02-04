#pragma once

#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <memory>
#include <string_view>
#include <list>
#include "core.h"
#include "half.h"

namespace IO
{
	namespace Util
	{
		size_t Align(size_t value, size_t alignment);
	}

	namespace Endian
	{
		inline int16_t Swap(int16_t value)
		{
			return (value << 8 & 0xFF00) | (value >> 8 & 0xFF);
		}

		inline int32_t Swap(int32_t value)
		{
			return (value >> 24 & 0xFF) | ((value >> 16 & 0xFF) << 8) | ((value >> 8 & 0xFF) << 16) | ((value & 0xFF) << 24);
		}

		// HACK!!!
		inline uint16_t Swap(uint16_t value)
		{
			int16_t temp = Swap(*(int16_t*)&value);
			return *(uint16_t*)&temp;
		}

		inline uint32_t Swap(uint32_t value)
		{ 
			int32_t temp = Swap(*(int32_t*)&value);
			return *(uint32_t*)&temp;
		}

		inline float Swap(float value)
		{
			float temp = 0.0f;
			char *src = (char*)&value, *dst = (char*)&temp;

			dst[0] = src[3];
			dst[1] = src[2];
			dst[2] = src[1];
			dst[3] = src[0];

			return temp;
		}
	}

	namespace Path
	{
		std::string GetFilename(std::string_view path);
		size_t GetSegmentCount(std::string_view path);
		std::string GetSegment(std::string_view path, int32_t segmentIndex);
		std::string GetUntilSegment(std::string_view path, int32_t lastSegIndex);
		std::string Combine(std::string_view left, std::string_view right);

		inline bool IsPathSeparator(const char c) { return (c == '/') || (c == '\\'); }
		inline bool IsDrive(std::string_view path)
		{ 
			return (GetSegmentCount(path) == 1) && 
				   ((path[0] > 'A' && path[0] < 'Z') || (path[0] > 'a' && path[0] < 'z')) && 
				   (path[1] == ':');
		}
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
		FileBuffer ReadAllData(std::string_view path, bool nullTerminated = false);
		bool Exists(std::string_view path);
	}

	namespace Directory
	{
		bool Create(std::string_view path);
		bool Exists(std::string_view path);
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
		inline size_t GetRemaining() { return mSize - mPosition; }
		inline void SetEndianness(Endianness endian) { mEndianness = endian; }

		inline void PushBaseOffset() { BaseOffsets.push_back(mPosition); }
		inline void PopBaseOffset()
		{
			if (BaseOffsets.size() > 0)
				BaseOffsets.pop_back();
		}

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

		inline int16_t ReadInt16() { return ShouldSwap() ? ReadI16_B() : Internal_ReadT<int16_t>(); }
		inline uint16_t ReadUInt16() { return ShouldSwap() ? ReadU16_B() : Internal_ReadT<uint16_t>(); }
		inline int32_t ReadInt32() { return ShouldSwap() ? ReadI32_B() : Internal_ReadT<int32_t>(); }
		inline uint32_t ReadUInt32() { return ShouldSwap() ? ReadU32_B() : Internal_ReadT<uint32_t>(); }
		inline float ReadFloat32() { return ShouldSwap() ? ReadF32_B() : Internal_ReadT<float>(); }

		// Read null-terminated string
		std::string ReadString();
		inline std::string ReadStringOffset()
		{
			std::string str;
			ReadAtOffset(ReadUInt32(), [&str](IO::MemoryReader& reader) { str = reader.ReadString(); });
			return str;
		}
		
		void ReadAtOffset(size_t offset, std::function<void(IO::MemoryReader&)> task, bool useBaseOffset = false);
	private:
		std::vector<size_t> BaseOffsets;
		std::unique_ptr<uint8_t[]> mContent;
		size_t mSize = 0;
		size_t mPosition = 0;
		Endianness mEndianness = Endianness::Little;

		inline bool ShouldSwap() { return mEndianness == Endianness::Big; }

		// Internal use only
		inline int16_t ReadI16_B() { return Endian::Swap(Internal_ReadT<int16_t>()); }
		inline uint16_t ReadU16_B() { return Endian::Swap(Internal_ReadT<uint16_t>()); }
		inline int32_t ReadI32_B() { return Endian::Swap(Internal_ReadT<int32_t>()); }
		inline uint32_t ReadU32_B() { return Endian::Swap(Internal_ReadT<uint32_t>()); }
		inline float ReadF32_B() { return Endian::Swap(Internal_ReadT<float>()); }

		// Basic types only
		template <typename T>
		inline T Internal_ReadT()
		{
			T value;
			Read(&value, sizeof(T));
			return value;
		}
	};

	// NOTE: This is necessary for the schedule functions
	class MemoryWriter;
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
		inline size_t GetPosition() { return mPosition; }
		inline size_t GetSize() { return mSize; }
		inline const void* GetData() { return mContent.get(); }

		inline void Seek(size_t pos) { mPosition = pos; }
		inline void SeekEnd(size_t pos) { mPosition = mSize - pos; }

		inline void PushBaseOffset() { BaseOffsets.push_back(mPosition); }
		inline void PopBaseOffset()
		{
			if (BaseOffsets.size() > 0)
				BaseOffsets.pop_back();
		}

		bool Write(const void* buffer, size_t size);
		inline void WriteChar(char value) { Write(&value, 1); }
		inline void WriteInt8(int8_t value) { Write(&value, 1); }
		inline void WriteUInt8(uint8_t value) { Write(&value, 1); }
		inline void WriteInt16(int16_t value) { ShouldSwap() ? WriteI16_B(value) : Write(&value, 2); }
		inline void WriteUInt16(uint16_t value) { ShouldSwap() ? WriteU16_B(value) : Write(&value, 2); }
		inline void WriteInt32(int32_t value) { ShouldSwap() ? WriteI32_B(value) : Write(&value, 4); }
		inline void WriteUInt32(uint32_t value) { ShouldSwap() ? WriteU32_B(value) : Write(&value, 4); }
		inline void WriteFloat16(float value) { ShouldSwap() ? WriteF16_B(value) : WriteF16_L(value); }
		inline void WriteFloat32(float value) { ShouldSwap() ? WriteF32_B(value) : Write(&value, 4); }
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

		void ScheduleWrite(std::function<void(IO::MemoryWriter&)> task);
		void ScheduleWriteOffset(std::function<void(IO::MemoryWriter&)> task, size_t baseOffset = 0);
		void ScheduleWriteOffsetAndSize(std::function<void(IO::MemoryWriter&)> task, size_t baseOffset = 0);
		void ScheduleWriteStringOffset(const std::string_view data, size_t baseOffset = 0);
		void FlushScheduledWrites();
		void FlushScheduledStrings();

		bool Flush(std::string_view path);
		bool CopyTo(MemoryWriter& destination);
	private:
		struct ScheduledWrite
		{
			int32_t OffsetPosition = -1;
			int32_t SizePosition = -1;
			size_t BaseOffset = 0;
			std::function<void(IO::MemoryWriter&)> Task;
		};

		struct ScheduledString
		{
			int32_t OffsetPosition = -1;
			size_t BaseOffset = 0;
			std::string Data;
		};

		std::vector<size_t> BaseOffsets;
		std::list<ScheduledWrite> ScheduledWrites;
		std::list<ScheduledString> ScheduledStrings;
		std::unique_ptr<uint8_t[]> mContent;
		size_t mSize, mPosition, mCapacity;
		Endianness mEndianness;
		const size_t mBufferSize = 256 * 1024; // 256kb


		inline bool ShouldSwap() { return mEndianness == Endianness::Big; }

		// Internal use only
		inline bool WriteI16_B(int16_t value) { int16_t v = Endian::Swap(value); Write(&v, 2); return true; }
		inline bool WriteU16_B(uint16_t value) { uint16_t v = Endian::Swap(value); Write(&v, 2); return true; }
		inline bool WriteI32_B(int32_t value) { int32_t v = Endian::Swap(value); Write(&v, 4); return true; }
		inline bool WriteU32_B(uint32_t value) { uint32_t v = Endian::Swap(value); Write(&v, 4); return true; }
		inline bool WriteF16_L(float value) { FLOAT16 v = FLOAT16::ToFloat16(value); Write(&v, 2); return true; }
		inline bool WriteF16_B(float value)
		{
			FLOAT16 v = FLOAT16::ToFloat16(value);
			uint16_t swapValue = Endian::Swap(*(uint16_t*)&v);
			Write(&swapValue, 2);
			return true;
		}
		inline bool WriteF32_B(float value) { float v = Endian::Swap(value); Write(&v, 4); return true; }

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