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
		CanonicalProperties() = default;
		~CanonicalProperties() = default;

		void Parse(const char* buffer, size_t size);

		bool OpenScope(std::string_view scope);
		bool CloseScope();
		const KeyValue* FindByKey(std::string_view key) const;
		const KeyValue* FindByKeyScoped(std::string_view key) const;
		bool Read(std::string_view key, std::string& value) const;
		bool Read(std::string_view key, int32_t& value, bool hex = false) const;
		bool Read(std::string_view key, float& value) const;

		template <typename T>
		inline bool ReadEnum(std::string_view key, T& value, const char* const* rep) const
		{
			std::string src;
			if (!Read(key, src)) return false;

			for (int32_t i = 0; i < static_cast<int32_t>(T::Count); i++)
				if (strcmp(src.c_str(), rep[i]) == 0)
					value = static_cast<T>(i);
			return true;
		}

		// Writing
		void Add(std::string_view key, std::string_view value);
		inline void Add(std::string_view key, int32_t value)
		{
			char buffer[0x20] = { '\0' };
			sprintf_s(buffer, 0x20, "%d", value);
			Add(key, buffer);
		}

		inline void Add(std::string_view key, float value)
		{
			char buffer[0x20] = { '\0' };
			sprintf_s(buffer, 0x20, "%f", value);
			Add(key, buffer);
		}

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

		// NOTE: Using char array instead of std::string for this because
		//       this may be changed constantly and std::string isn't very
		//       optimized for that (and also makes it easier to format and
		//       erase scope paths)
		char mScope[0x80] = { '\0' };
		std::vector<int32_t> mScopeStepStack;

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
