#pragma once

#include <stdint.h>
#include <string_view>

namespace Util
{
	namespace String
	{
		int32_t GetIndex(std::string_view str, char seek);
		int32_t GetLastIndex(std::string_view str, char seek);
	}
}
