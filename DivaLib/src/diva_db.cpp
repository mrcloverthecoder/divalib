#include "pch.h"
#include "diva_db.h"

using namespace Database;

void SpriteDatabase::Parse(IO::Reader& reader)
{
	int32_t setCount = reader.ReadInt32();
	int32_t setOffset = reader.ReadInt32();
	int32_t dataCount = reader.ReadInt32(); // Sprite and textures
	int32_t dataOffset = reader.ReadInt32();

	reader.SeekBegin(setOffset);
	for (int i = 0; i < setCount; i++)
	{
		// Add a new entry
		SpriteSetInfo& info = Sets.emplace_back();
		// Read data
		info.Id = reader.ReadUInt32();
		info.Name = reader.ReadStringOffset();
		info.Filename = reader.ReadStringOffset();
		info.Index = reader.ReadInt32();
	}

	reader.SeekBegin(dataOffset);
	for (int i = 0; i < dataCount; i++)
	{
		// Add a new entry
		SpriteDataInfo& info = Data.emplace_back();
		// Read data
		info.Id = reader.ReadUInt32();
		info.Name = reader.ReadStringOffset();
		info.SetId = reader.ReadInt32();
	}
}

const SpriteDataInfo* SpriteDatabase::FindSpriteById(uint32_t id) const
{
	for (const SpriteDataInfo& info : Data)
		if (info.Id == id && !info.IsTexture())
			return &info;

	return nullptr;
}

const SpriteDataInfo* SpriteDatabase::FindTextureById(uint32_t id) const
{
	for (const SpriteDataInfo& info : Data)
		if (info.Id == id && info.IsTexture())
			return &info;

	return nullptr;
}

const SpriteSetInfo* SpriteDatabase::GetSpriteSetByIndex(int32_t index) const
{
	if (index > -1 && index < Sets.size())
		return &Sets.at(index);

	return nullptr;
}