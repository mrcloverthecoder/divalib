#pragma once

#include <string>
#include <vector>
#include "core_io.h"

namespace Database
{
	struct SpriteDataInfo
	{
		uint32_t Id = 0;
		std::string Name;
		// SetId tightly packs set index, data index and texture flag
		// DataIndex   = (SetId & 0xFFFF)
		// SetIndex    = (SetId >> 0x10 & 0xFFF)
		// TextureFlag = ((SetId >> 0x10 & 0x1000) != 0)
		int32_t DataIndex = -1;

		/*
		inline int32_t GetSetIndex() const { return (SetId >> 0x10 & 0xFFF); }
		inline int32_t GetIndex() const { return (SetId & 0xFFFF); }
		inline bool IsTexture() const { return ((SetId >> 0x10 & 0x1000) != 0); }*/
	};

	struct SpriteSetInfo
	{
		uint32_t Id = 0;
		int32_t Index = -1;
		std::string Name;
		std::string Filename;
		std::vector<SpriteDataInfo> Sprites;
		std::vector<SpriteDataInfo> Textures;
	};

	class SpriteDatabase
	{
	public:
		std::vector<SpriteSetInfo> SpriteSets;

		void Parse(IO::Reader& reader);
		void Write(IO::Writer& writer);

		SpriteSetInfo* FindSpriteSetByName(std::string_view name);
		const SpriteDataInfo* FindSpriteById(uint32_t id) const;
		const SpriteDataInfo* FindTextureById(uint32_t id) const;
		const SpriteSetInfo* GetSpriteSetByIndex(int32_t index) const;
	};
}
