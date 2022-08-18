#pragma once

#include <stdint.h>
#include <string_view>

namespace Util
{
	namespace String
	{
		int32_t GetLastIndex(std::string_view str, char seek);
		bool StartsWith(std::string_view str, std::string_view compare);
		bool EndsWith(std::string_view str, std::string_view compare);
	}
}
