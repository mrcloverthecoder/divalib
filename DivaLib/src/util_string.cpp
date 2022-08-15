#include "pch.h"
#include "util_string.h"

int32_t Util::String::GetLastIndex(std::string_view str, char seek)
{
	int32_t index = -1;
	const char* ptr = str.data();

	// Loop through string and try to find the char being seeked
	while (*ptr)
	{
		if (*ptr == seek)
			index = ptr - str.data();
		ptr++;
	}

	return index;
}