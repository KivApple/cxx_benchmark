#include <iostream>
#include <chrono>
#include <fstream>
#include "icosphere.h"
#include "pf2_font_loader.h"

size_t test_icosphere() {
	auto [vertexes, triangles] = generateMesh(4);
	return vertexes.size() * triangles.size();
}

size_t test_font_loader(std::string_view fontData) {
	PF2FontLoader loader("font", fontData);
	auto [textureData, charMap] = loader.load();
	return textureData.size() * charMap.size();
}

int main() {
	auto start = std::chrono::steady_clock::now();
	auto count = 10000;
	for (int i = 0; i < count; i++) {
		(void) test_icosphere();
	}
	auto elapsed = std::chrono::steady_clock::now() - start;
	std::cout << "Mesh generation time: " <<
		(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / count) <<" us" << std::endl;
	
	std::string fontData;
	{
		std::ifstream f("assets/DroidSans-32.pf2", std::ios::binary);
		f.seekg(0, std::ios::end);
		fontData.resize(f.tellg());
		f.seekg(0);
		f.read(fontData.data(), fontData.size());
	}
	
	start = std::chrono::steady_clock::now();
	count = 1000;
	for (int i = 0; i < count; i++) {
		(void) test_font_loader(fontData);
	}
	elapsed = std::chrono::steady_clock::now() - start;
	std::cout << "Font loading time: " <<
			  (std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / count) <<" us" << std::endl;
	
	return 0;
}
