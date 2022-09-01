#include "pch.h"
#include <stdio.h>
#include <algorithm>
#include "diva_prop.h"
#include "util_string.h"

using namespace Property;

void CanonicalProperties::Parse(const char* buffer, size_t size)
{
	mContent = std::string(buffer, size);

	while (mPosition < mContent.size())
	{
		std::string_view line = GetNextLine();
		int32_t sep = Util::String::GetIndex(line, '=');
		if (line.empty() || line[0] == '#' || sep < 1)
			continue;

		std::string_view key = line.substr(0, sep);
		// NOTE: (line.size() - sep - 2) = size of line - index of '=' - 1 ('=') - 1 ('\n') 
		std::string_view val = line.substr(sep + 1, line.size() - sep - 2);
		mRanges.push_back(std::make_pair(key, val));
	}
}

KeyValue* CanonicalProperties::FindByKey(std::string_view key)
{
	size_t low = 0;
	size_t high = mRanges.size() - 1; // Highest index

	while (low <= high)
	{
		size_t mid = (low + high) / 2;
		int result = strncmp(mRanges[mid].first.data(), key.data(), key.size());

		if (result == 0) return &mRanges[mid];
		else if (result < 0) low = mid + 1;
		else if (result > 0) high = mid - 1;
	}

	return nullptr;
}

KeyValue* CanonicalProperties::FindByKeyScoped(std::string_view key)
{
	return FindByKey(key);
}

bool CanonicalProperties::Read(std::string_view key, std::string& value)
{
	const auto* kv = FindByKeyScoped(key);
	if (!kv) return false;
	value = std::string(kv->second.data(), kv->second.size());
	return true;
}

bool CanonicalProperties::Read(std::string_view key, int32_t& value, bool hex)
{
	std::string src;
	if (!Read(key, src)) return false;
	value = strtol(src.data(), nullptr, hex ? 16 : 10);
	return true;
}

void CanonicalProperties::Add(std::string_view key, std::string_view value)
{
	size_t keyOffset = mContent.size();
	mContent += key;
	size_t valueOffset = mContent.size();
	mContent += value;

	// Create ranges
	std::string_view keyView(mContent.data() + keyOffset, key.size());
	std::string_view valueView(mContent.data() + valueOffset, value.size());
	mRanges.push_back(std::make_pair(keyView, valueView));
	mRangeMarkups.push_back({ keyOffset, key.size(), valueOffset, value.size() });
	// Rearrange all views
	Rearrange();
}

void CanonicalProperties::Write(IO::Writer& writer)
{
	// I didn't think std::sort would just work:tm:
	// out of the box like this, but, it did! I love STL
	std::sort(mRanges.begin(), mRanges.end());

	for (const auto& range : mRanges)
	{
		writer.Write(range.first.data(), range.first.size()); // Key
		writer.WriteChar('=');
		writer.Write(range.second.data(), range.second.size()); // Value
		writer.WriteChar('\n');
	}
}

void CanonicalProperties::Rearrange()
{
	if (mRanges.size() != mRangeMarkups.size())
		return;

	for (size_t i = 0; i < mRanges.size(); i++)
	{
		const RangeMarkup& mark = mRangeMarkups[i];
		auto& range = mRanges[i];
		
		range.first = std::string_view(mContent.data() + mark.KeyOffset, mark.KeySize);
		range.second = std::string_view(mContent.data() + mark.ValueOffset, mark.ValueSize);
	}
}