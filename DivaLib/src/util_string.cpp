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

bool Util::String::StartsWith(std::string_view str, std::string_view compare)
{
	const char* orgPtr = str.data();
	const char* cmpPtr = compare.data();

	while (*orgPtr && *cmpPtr)
	{
		if (*orgPtr != *cmpPtr)
			return false;

		orgPtr++;
		cmpPtr++;
	}

	return true;
}

bool Util::String::EndsWith(std::string_view str, std::string_view compare)
{
	const char* orgPtr = &str.back();
	const char* cmpPtr = &compare.back();

	while (0 < orgPtr - str.data() && 0 < cmpPtr - compare.data())
	{
		if (*orgPtr != *cmpPtr)
			return false;

		orgPtr--;
		cmpPtr--;
	}

	return true;
}