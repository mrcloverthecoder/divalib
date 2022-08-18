#pragma once

#include <string>
#include <vector>
#include "io_core.h"

namespace Database
{
	struct SpriteSetInfo
	{
		uint32_t Id;
		std::string Name;
		std::string Filename;
		int32_t Index;
	};

	struct SpriteDataInfo
	{
		uint32_t Id;
		std::string Name;
		// SetId tightly packs set index, data index and texture flag
		// DataIndex   = (SetId & 0xFFFF)
		// SetIndex    = (SetId >> 0x10 & 0xFFF)
		// TextureFlag = ((SetId >> 0x10 & 0x1000) != 0)
		int32_t SetId;

		inline int32_t GetSetIndex() const { return (SetId >> 0x10 & 0xFFF); }
		inline int32_t GetIndex() const { return (SetId & 0xFFFF); }
		inline bool IsTexture() const { return ((SetId >> 0x10 & 0x1000) != 0); }
	};

	class SpriteDatabase
	{
	public:
		std::vector<SpriteSetInfo> Sets;
		std::vector<SpriteDataInfo> Data;

		void Parse(IO::Stream& stream);

		const SpriteDataInfo* FindSpriteById(uint32_t id) const;
		const SpriteDataInfo* FindTextureById(uint32_t id) const;
		const SpriteSetInfo* GetSpriteSetByIndex(int32_t index) const;
	};
}
