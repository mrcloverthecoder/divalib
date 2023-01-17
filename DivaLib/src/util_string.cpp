#include "pch.h"
#include "util_string.h"

int32_t Util::String::GetIndex(std::string_view str, char seek)
{
	for (size_t i = 0; i < str.size(); i++)
		if (str[i] == seek)
			return i;

	return -1;
}

int32_t Util::String::GetLastIndex(std::string_view str, char seek)
{
	if (str.empty())
		return -1;

	int32_t index = -1;
	for (size_t i = 0; i < str.size(); i++)
		if (str[i] == seek)
			index = static_cast<int32_t>(i);

	return index;
}

int32_t Util::String::Count(std::string_view str, char seek)
{
	int32_t count = 0;

	for (const char& c : str)
		if (c == seek)
			count++;

	return count;
}

std::string Util::String::ToLower(std::string_view str)
{
	std::string lower;
	for (const char& c : str)
		lower += tolower(c);
	return std::move(lower);
}