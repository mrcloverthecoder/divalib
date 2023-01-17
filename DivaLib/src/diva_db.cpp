#include "pch.h"
#include "diva_db.h"

using namespace Database;

void SpriteDatabase::Parse(IO::Reader& reader)
{
	uint32_t setCount = reader.ReadUInt32();
	uint32_t setOffset = reader.ReadUInt32();
	uint32_t dataCount = reader.ReadUInt32(); // Sprite and textures
	uint32_t dataOffset = reader.ReadUInt32();

	SpriteSets.reserve(setCount);

	reader.ExecuteAtOffset(setOffset, [&reader, setCount, this]
	{
		for (size_t i = 0; i < setCount; i++)
		{
			SpriteSetInfo& info = SpriteSets.emplace_back();
			info.Id = reader.ReadUInt32();
			info.Name = reader.ReadStringOffset();
			info.Filename = reader.ReadStringOffset();
			info.Index = reader.ReadInt32();
		}
	});

	reader.ExecuteAtOffset(dataOffset, [&reader, dataCount, this]
	{
		for (int i = 0; i < dataCount; i++)
		{
			uint32_t id = reader.ReadUInt32();
			std::string name = reader.ReadStringOffset();
			// NOTE: Bit-packed sprite data
			int32_t setId = reader.ReadInt32();

			int32_t dataIndex = setId & 0xFFFF;
			int32_t setIndex = setId >> 0x10 & 0xFFF;
			bool texFlag = (setId >> 0x10 & 0x1000) == 0x1000;

			SpriteSetInfo& set = SpriteSets[setIndex];
			SpriteDataInfo& data = texFlag ? set.Textures.emplace_back() : set.Sprites.emplace_back();
			data.Id = id;
			data.Name = std::move(name);
			data.DataIndex = dataIndex;
		}
	});
}

void SpriteDatabase::Write(IO::Writer& writer)
{
	writer.WriteUInt32(SpriteSets.size());
	writer.ScheduleWriteOffset([this](IO::Writer& writer)
	{
		int32_t index = 0;
		for (auto& set : SpriteSets)
		{
			writer.WriteUInt32(set.Id);
			writer.ScheduleWriteStringOffset(set.Name);
			writer.ScheduleWriteStringOffset(set.Filename);
			writer.WriteInt32(index++);
		}
	});

	size_t dataCount = 0;
	for (auto& set : SpriteSets)
	{
		dataCount += set.Sprites.size();
		dataCount += set.Textures.size();
	}

	writer.WriteUInt32(dataCount);
	writer.ScheduleWriteOffset([this](IO::Writer& writer)
	{
		int32_t setIndex = 0;
		for (auto& set : SpriteSets)
		{
			for (auto& spr : set.Sprites)
			{
				writer.WriteUInt32(spr.Id);
				writer.ScheduleWriteStringOffset(spr.Name);
				int32_t setId = (spr.DataIndex & 0xFFFF) | ((setIndex & 0xFFF) << 0x10);
				writer.WriteInt32(setId);
			}

			for (auto& tex : set.Textures)
			{
				writer.WriteUInt32(tex.Id);
				writer.ScheduleWriteStringOffset(tex.Name);
				int32_t setId = (tex.DataIndex & 0xFFFF) | ((setIndex & 0xFFF) << 0x10) | 0x10000000;
				writer.WriteInt32(setId);
			}

			setIndex++;
		}
	});

	writer.FlushScheduledWrites();
	writer.FlushScheduledStrings();
}

SpriteSetInfo* SpriteDatabase::FindSpriteSetByName(std::string_view name)
{
	for (auto& set : SpriteSets)
		if (strncmp(name.data(), set.Name.c_str(), name.size()) == 0)
			return &set;

	return nullptr;
}

/*
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
*/