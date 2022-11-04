#include <cassert>
#include <cmath>
#include "pf2_font_loader.h"

struct PF2CharIndexItem {
	uint32_t unicodeCodePoint;
	uint8_t storageFlags;
	uint32_t offset;
	
	template<typename S> void serialize(S &s) {
		s.value4b(unicodeCodePoint);
		s.value1b(storageFlags);
		s.value4b(offset);
	}
};

struct PF2CharHeader {
	uint16_t width;
	uint16_t height;
	int16_t xOffset;
	int16_t yOffset;
	int16_t deviceWidth;
	
	template<typename S> void serialize(S &s) {
		s.value2b(width);
		s.value2b(height);
		s.value2b(xOffset);
		s.value2b(yOffset);
		s.value2b(deviceWidth);
	}
};

struct PF2FontLoader::AdapterConfig: public bitsery::DefaultConfig {
	static constexpr bitsery::EndiannessType Endianness = bitsery::EndiannessType::BigEndian;
};

PF2FontLoader::PF2FontLoader(
		std::string_view name,
		std::string_view data
): m_name(name), m_data(data), m_deserializer(Adapter(data.begin(), data.end())) {
}

bool PF2FontLoader::readSection() {
	if (m_deserializer.adapter().error() == bitsery::ReaderError::NoError) {
		m_deserializer.adapter().currentReadPos(m_nextSectionPosition);
	}
	
	static constexpr size_t TAG_LENGTH = 4;
	if (m_deserializer.adapter().currentReadPos() + TAG_LENGTH <= m_data.size()) {
		m_curSectionTag = m_data.substr(m_deserializer.adapter().currentReadPos(), TAG_LENGTH);
	} else {
		m_curSectionTag = {};
	}
	m_deserializer.adapter().currentReadPos(m_deserializer.adapter().currentReadPos() + m_curSectionTag.size());
	m_deserializer.value4b(m_curSectionLength);
	if (m_deserializer.adapter().error() == bitsery::ReaderError::NoError && m_curSectionLength != 0xFFFFFFFF) {
		m_nextSectionPosition = m_deserializer.adapter().currentReadPos() + m_curSectionLength;
	} else {
		m_nextSectionPosition = 0;
	}
	return m_nextSectionPosition > 0;
}

std::string_view PF2FontLoader::sectionAsText() {
	if (m_curSectionLength > 1) {
		return m_data.substr(m_deserializer.adapter().currentReadPos(), m_curSectionLength - 1);
	} else {
		return {};
	}
}

uint16_t PF2FontLoader::readUInt16() {
	uint16_t value = 0;
	m_deserializer.value2b(value);
	return value;
}

void PF2FontLoader::parseCharIndex() {
	auto endPos = m_deserializer.adapter().currentReadPos() + m_curSectionLength;
	PF2CharIndexItem item = {};
	m_charIndexes.reserve(m_curSectionLength / sizeof(PF2CharIndexItem));
	while (
			m_deserializer.adapter().currentReadPos() < endPos &&
			m_deserializer.adapter().error() == bitsery::ReaderError::NoError
	) {
		m_deserializer.object(item);
		if ((item.storageFlags & 0b111) != 0) {
			/* spdlog::error(
					"PF2 font character with Unicode point {} has compressed storage type which is not supported",
					item.unicodeCodePoint
			); */
			m_deserializer.adapter().error(bitsery::ReaderError::InvalidData);
			break;
		}
		m_charIndexes.emplace(item.unicodeCodePoint, std::make_pair(item.offset, m_charMap.size()));
	}
	/* spdlog::trace(
			"PF2 font character index has {} Unicode code point(s) ({} unique glyph(s))",
			m_charMap.size(),
			m_charIndexes.size()
	); */
}

void PF2FontLoader::parseSection() {
	if (!m_started) {
		if (m_curSectionTag == "FILE") {
			auto magic = m_data.substr(
					m_deserializer.adapter().currentReadPos(),
					m_curSectionLength
			);
			if (magic == "PFF2") {
				m_started = true;
				//spdlog::trace("PF2 font \"{}\"", m_name);
				return;
			}
		}
		m_deserializer.adapter().error(bitsery::ReaderError::InvalidData);
		//spdlog::error("PF2 font \"{}\" has invalid file format", m_name);
	}
	if (m_curSectionTag == "NAME" && m_curSectionLength > 1) {
		//spdlog::trace("PF2 font name: {}", sectionAsText());
	} else if (m_curSectionTag == "FAMI" && m_curSectionLength > 1) {
		//spdlog::trace("PF2 font family: {}", sectionAsText());
	} else if (m_curSectionTag == "WEIG" && m_curSectionLength > 1) {
		//spdlog::trace("PF2 font weight: {}", sectionAsText());
	} else if (m_curSectionTag == "SLAN" && m_curSectionLength > 1) {
		//spdlog::trace("PF2 font slant: {}", sectionAsText());
	} else if (m_curSectionTag == "PTSZ" && m_curSectionLength == 2) {
		m_pointSize = readUInt16();
		//spdlog::trace("PF2 font point size: {}", m_pointSize);
	} else if (m_curSectionTag == "MAXW" && m_curSectionLength == 2) {
		m_maxCharWidth = readUInt16();
		//spdlog::trace("PF2 font max char width: {}", m_maxCharWidth);
	} else if (m_curSectionTag == "MAXH" && m_curSectionLength == 2) {
		m_maxCharHeight = readUInt16();
		//spdlog::trace("PF2 font max char height: {}", m_maxCharHeight);
	} else if (m_curSectionTag == "ASCE" && m_curSectionLength == 2) {
		m_ascent = readUInt16();
		//spdlog::trace("PF2 font ascent: {}", m_ascent);
	} else if (m_curSectionTag == "DESC" && m_curSectionLength == 2) {
		m_descent = readUInt16();
		//spdlog::trace("PF2 font descent: {}", m_descent);
	} else if (m_curSectionTag == "CHIX") {
		parseCharIndex();
	} else {
		//spdlog::trace("Skipping PF2 font section \"{}\" ({} byte(s))", m_curSectionTag, m_curSectionLength);
	}
}

void PF2FontLoader::parseCharBitmap(int index, const PF2CharHeader &header, FontGlyphInfo &glyph) {
	int x0 = (index % m_colCount) * m_maxCharWidth;
	int y0 = (index / m_colCount) * m_maxCharHeight;
	
	glyph.texCoord.x = static_cast<float>(x0) / static_cast<float>(m_textureWidth);
	glyph.texCoord.y = static_cast<float>(y0) / static_cast<float>(m_textureHeight);
	glyph.texSize.x = static_cast<float>(header.width) / static_cast<float>(m_textureWidth);
	glyph.texSize.y = static_cast<float>(header.height) / static_cast<float>(m_textureHeight);
	glyph.offset.x = static_cast<float>(header.xOffset) / static_cast<float>(m_pointSize);
	glyph.offset.y = static_cast<float>(header.yOffset) / static_cast<float>(m_pointSize);
	glyph.size.x = static_cast<float>(header.width) / static_cast<float>(m_pointSize);
	glyph.size.y = static_cast<float>(header.height) / static_cast<float>(m_pointSize);
	glyph.width = static_cast<float>(header.deviceWidth) / static_cast<float>(m_pointSize);
	
	auto base = m_deserializer.adapter().currentReadPos();
	for (int y = 0; y < header.height; y++) {
		int j = (y0 + y) * m_textureWidth + x0;
		for (int x = 0; x < header.width; x++) {
			int i = y * header.width + x;
			uint8_t byte = m_data[base + i / 8];
			if ((byte & (1 << (7 - i % 8))) != 0) {
				m_textureData[j + x] = {255, 255, 255, 255};
			}
		}
	}
}

void PF2FontLoader::parseDataSection() {
	m_colCount = static_cast<int>(std::ceil(std::sqrt(
			static_cast<float>(m_charIndexes.size()) *
			static_cast<float>(m_maxCharHeight) /
			static_cast<float>(m_maxCharWidth)
	)));
	m_textureWidth = m_colCount * m_maxCharWidth;
	m_textureHeight = static_cast<int>((m_charIndexes.size() + m_colCount - 1) / m_colCount * m_maxCharHeight);
	m_textureData.resize(m_textureWidth * m_textureHeight);
	m_charMap.reserve(m_charIndexes.size());
	PF2CharHeader header = {};
	for (auto &item : m_charIndexes) {
		m_deserializer.adapter().currentReadPos(item.second.first);
		m_deserializer.object(header);
		FontGlyphInfo glyphInfo = {};
		parseCharBitmap(item.second.second, header, glyphInfo);
		m_charMap.emplace(item.first, glyphInfo);
	}
}

std::tuple<
		std::vector<glm::vec<4, uint8_t>>,
		absl::flat_hash_map<int, FontGlyphInfo>
> PF2FontLoader::load() {
	assert(!m_started);
	while (readSection()) {
		parseSection();
	}
	if (!m_charIndexes.empty() && m_curSectionTag == "DATA") {
		parseDataSection();
		/* spdlog::debug(
				"Loaded PF2 font \"{}\" ({} characters(s), {} glyph(s), {}x{} generated texture)",
				m_name,
				m_charMap.size(),
				m_charIndexes.size(),
				m_textureWidth,
				m_textureHeight
		); */
		return std::make_tuple<
				std::vector<glm::vec<4, uint8_t>>,
				absl::flat_hash_map<int, FontGlyphInfo>
		>(
				std::move(m_textureData),
				std::move(m_charMap)
		);
	} else {
		//spdlog::error("PF2 font has no \"DATA\" section");
	}
	return std::make_tuple<
			std::vector<glm::vec<4, uint8_t>>,
			absl::flat_hash_map<int, FontGlyphInfo>
	>({}, {{0, {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}, 1.0f}}});
}
