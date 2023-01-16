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

bool CanonicalProperties::OpenScope(std::string_view scope)
{
	if (scope.empty())
		return false;

	if (mScope[0] != '\0')
		strcat_s(mScope, 0x80, ".");
	strncat_s(mScope, 0x80, scope.data(), scope.size());
	mScopeStepStack.push_back(Util::String::Count(scope, '.') + 1);
	return true;
}

bool CanonicalProperties::CloseScope()
{
	if (!mScope[0]) return false;

	for (int32_t i = 0; i < mScopeStepStack.back(); i++)
	{
		int32_t index = Util::String::GetLastIndex(mScope, '.');
		if (index < 0)
		{
			mScope[0] = '\0';
			break;
		}

		mScope[index] = '\0';
	}

	mScopeStepStack.pop_back();
	return true;
}

const KeyValue* CanonicalProperties::FindByKey(std::string_view key) const
{
	// NOTE: The algo will throw vector subscript error if
	//       this is unsigned and the key does not exist
	int32_t low = 0;
	int32_t high = static_cast<int32_t>(mRanges.size() - 1); // Highest index

	if (low > high)
		return nullptr;

	char keyData[0x80] = { 0x00 };
	while (low <= high)
	{
		int32_t mid = (low + high) / 2;
		const auto& srcKey = mRanges[mid].first;

		// HACK: This is just a placeholder fix. Should be changed later
		strncpy_s(keyData, 0x80, srcKey.data(), srcKey.size());
		int result = strncmp(keyData, key.data(), 0x80);

		if (result == 0) return &mRanges[mid];
		else if (result < 0) low = mid + 1;
		else if (result > 0) high = mid - 1;
	}

	return nullptr;
}

const KeyValue* CanonicalProperties::FindByKeyScoped(std::string_view key) const
{
	if (key.size() < 1)
		return nullptr;

	char scopeKey[0x80] = { '\0' };
	// NOTE: Copy scope string
	if (mScope[0] != '\0')
	{
		strcpy_s(scopeKey, 0x80, mScope);
		strcat_s(scopeKey, 0x80, ".");
	}
	// NOTE: Copy key string
	strncat_s(scopeKey, 0x80, key.data(), key.size());

	// NOTE: Try to find full key
	return FindByKey(scopeKey);
}

bool CanonicalProperties::Read(std::string_view key, std::string& value) const
{
	const auto* kv = FindByKeyScoped(key);
	if (!kv) return false;
	value = std::string(kv->second.data(), kv->second.size());
	return true;
}

bool CanonicalProperties::Read(std::string_view key, int32_t& value, bool hex) const
{
	std::string src;
	if (!Read(key, src)) return false;
	value = strtol(src.data(), nullptr, hex ? 16 : 10);
	return true;
}

bool CanonicalProperties::Read(std::string_view key, float& value) const
{
	std::string src;
	if (!Read(key, src)) return false;
	value = strtof(src.data(), nullptr);
	return true;
}

void CanonicalProperties::Add(std::string_view key, std::string_view value)
{
	size_t keyOffset = mContent.size();
	size_t keySize = key.size();
	if (mScope[0] != '\0')
	{
		mContent += mScope;
		mContent += '.';
		keySize += strlen(mScope) + 1;
	}

	mContent += key;
	size_t valueOffset = mContent.size();
	mContent += value;

	// Create ranges
	std::string_view keyView(mContent.data() + keyOffset, keySize);
	std::string_view valueView(mContent.data() + valueOffset, value.size());
	mRanges.push_back(std::make_pair(keyView, valueView));
	mRangeMarkups.push_back({ keyOffset, keySize, valueOffset, value.size() });
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