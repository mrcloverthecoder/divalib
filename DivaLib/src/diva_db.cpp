#include "pch.h"
#include "diva_db.h"

using namespace Database;

void SpriteDatabase::Parse(IO::Stream& stream)
{
	int32_t setCount = stream.ReadInt32();
	int32_t setOffset = stream.ReadInt32();
	int32_t dataCount = stream.ReadInt32(); // Sprite and textures
	int32_t dataOffset = stream.ReadInt32();

	stream.SeekBegin(setOffset);
	for (int i = 0; i < setCount; i++)
	{
		// Add a new entry
		SpriteSetInfo& info = Sets.emplace_back();
		// Read data
		info.Id = stream.ReadUInt32();
		stream.ReadNullStringOffset(info.Name);
		stream.ReadNullStringOffset(info.Filename);
		info.Index = stream.ReadInt32();
	}

	stream.SeekBegin(dataOffset);
	for (int i = 0; i < dataCount; i++)
	{
		// Add a new entry
		SpriteDataInfo& info = Data.emplace_back();
		// Read data
		info.Id = stream.ReadUInt32();
		stream.ReadNullStringOffset(info.Name);
		info.SetId = stream.ReadInt32();
	}
}