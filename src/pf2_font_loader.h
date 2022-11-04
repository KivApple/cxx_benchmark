#pragma once

#include <string_view>
#include <absl/container/flat_hash_map.h>
#include <tuple>
#include <vector>
#include "bitsery/bitsery.h"
#include "bitsery/adapter/buffer.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"
#include "bitsery_string_view.h"

struct FontGlyphInfo {
	glm::vec2 texCoord;
	glm::vec2 texSize;
	glm::vec2 offset;
	glm::vec2 size;
	float width;
};

struct PF2CharHeader;

class PF2FontLoader {
	struct AdapterConfig;
	using Adapter = bitsery::InputBufferAdapter<std::string_view, AdapterConfig>;
	
	std::string_view m_name;
	std::string_view m_data;
	bitsery::Deserializer<Adapter> m_deserializer;
	size_t m_nextSectionPosition = 0;
	std::string_view m_curSectionTag;
	uint32_t m_curSectionLength = 0;
	bool m_started = false;
	int m_pointSize = 0;
	int m_maxCharWidth = 0;
	int m_maxCharHeight = 0;
	int m_ascent = 0;
	int m_descent = 0;
	absl::flat_hash_map<int, FontGlyphInfo> m_charMap;
	absl::flat_hash_map<uint32_t, std::pair<uint32_t, size_t>> m_charIndexes;
	std::vector<glm::vec<4, uint8_t>> m_textureData;
	int m_colCount = 0;
	int m_textureWidth = 0;
	int m_textureHeight = 0;
	
	bool readSection();
	[[nodiscard]] std::string_view sectionAsText();
	[[nodiscard]] uint16_t readUInt16();
	void parseCharIndex();
	void parseSection();
	void parseCharBitmap(int index, const PF2CharHeader &header, FontGlyphInfo &glyph);
	void parseDataSection();

public:
	PF2FontLoader(std::string_view name, std::string_view data);
	
	[[nodiscard]] std::tuple<
		std::vector<glm::vec<4, uint8_t>>,
		absl::flat_hash_map<int, FontGlyphInfo>
	> load();
	
};
