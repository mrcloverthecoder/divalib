#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "core_io.h"

namespace Property
{
	using KeyValue = std::pair<std::string_view, std::string_view>;

	class CanonicalProperties
	{
	public:
		void Parse(const char* buffer, size_t size);
		KeyValue* FindByKey(std::string_view key);
		KeyValue* FindByKeyScoped(std::string_view key);

		bool Read(std::string_view key, std::string& value);
		bool Read(std::string_view key, int32_t& value, bool hex = false);

		// Writing
		void Add(std::string_view key, std::string_view value);
		void Write(IO::Writer& writer);
	private:
		struct RangeMarkup
		{
			size_t KeyOffset = 0;
			size_t KeySize = 0;
			size_t ValueOffset = 0;
			size_t ValueSize = 0;
		};

		std::string mContent;
		std::vector<KeyValue> mRanges;
		// NOTE: This is used for writing (until I find a better solution)
		std::vector<RangeMarkup> mRangeMarkups;

		size_t mPosition = 0;
		inline std::string_view GetNextLine()
		{
			char* begin = &mContent[mPosition];
			char* end = strchr(begin, '\n');
			if (end == nullptr)
			{
				size_t remaining = mContent.size() - mPosition + 1;
				mPosition += remaining;
				return std::string_view(begin, remaining);
			}

			size_t size = static_cast<size_t>(end - begin) + 1;
			mPosition += size;
			return std::string_view(begin, size);
		}

		void Rearrange();
	};
}
