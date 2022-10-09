#include <vector>
#include <unordered_map>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <iostream>
#include <chrono>

template <class T> inline void hashCombine(std::size_t& seed, const T& v) {
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T1, typename T2> struct PairHasher {
	size_t operator()(const std::pair<T1, T2> &pair) const {
		size_t seed = 0;
		hashCombine(seed, pair.first);
		hashCombine(seed, pair.second);
		return seed;
	}
};

using MeshEdgeCache = std::unordered_map<
		std::pair<unsigned int, unsigned int>,
		unsigned int,
		PairHasher<unsigned int, unsigned int>,
		std::equal_to<>
>;

unsigned int midVertexForEdge(
		MeshEdgeCache &cache,
		std::vector<glm::vec3> &vertexes,
		unsigned int first,
		unsigned int second
) {
	std::pair<unsigned int, unsigned int> key(first, second);
	if (key.first > key.second) {
		std::swap(key.first, key.second);
	}
	auto it = cache.find(key);
	if (it == cache.end()) {
		it = cache.emplace_hint(it, key, vertexes.size());
		vertexes.emplace_back(glm::normalize(vertexes[first] + vertexes[second]));
	}
	return it->second;
}

std::vector<glm::vec<3, unsigned int>> subdivideMesh(
		std::vector<glm::vec3> &vertexes,
	const std::vector<glm::vec<3, unsigned int>> &triangles
) {
	std::vector<glm::vec<3, unsigned int>> result;
	MeshEdgeCache cache;
	for (auto &triangle : triangles) {
	unsigned int mid[] = {
			midVertexForEdge(cache, vertexes, triangle[0], triangle[1]),
			midVertexForEdge(cache, vertexes, triangle[1], triangle[2]),
			midVertexForEdge(cache, vertexes, triangle[2], triangle[0])
	};
	result.emplace_back(triangle[0], mid[0], mid[2]);
	result.emplace_back(triangle[1], mid[1], mid[0]);
	result.emplace_back(triangle[2], mid[2], mid[1]);
	result.emplace_back(mid[0], mid[1], mid[2]);
	}
	return result;
}

std::pair<std::vector<glm::vec3>, std::vector<glm::vec<3, unsigned int>>> generateMesh(int subdivisionCount) {
	static constexpr float X = 0.525731112119133606f;
	static constexpr float Z = 0.850650808352039932f;
	static constexpr float N = 0.0f;
	
	std::vector<glm::vec3> vertexes = {
			{-X, N, Z}, {X, N,Z}, {-X, N, -Z}, {X, N, -Z},
			{N, Z, X}, {N, Z,-X}, {N,-Z,X}, {N,-Z,-X},
			{Z, X, N}, {-Z, X, N}, {Z,-X,N}, {-Z,-X, N}
	};
	std::vector<glm::vec<3, unsigned int>> triangles = {
			{0, 4, 1}, {0, 9, 4}, {9, 5, 4}, {4, 5, 8}, {4, 8, 1},
			{8, 10, 1}, {8, 3, 10}, {5, 3, 8}, {5, 2, 3}, {2, 7, 3},
			{7, 10, 3}, {7, 6, 10}, {7, 11, 6}, {11, 0, 6}, {0, 1, 6},
			{6, 1, 10}, {9, 0, 11}, {9, 11, 2}, {9, 2, 5}, {7, 2, 11}
	};
	
	for (int i = 0; i < subdivisionCount; i++) {
		triangles = subdivideMesh(vertexes, triangles);
	}
	
	return std::make_pair(vertexes, triangles);
}

size_t test() {
	auto [vertexes, triangles] = generateMesh(4);
	return vertexes.size() * triangles.size();
}

int main() {
	auto start = std::chrono::steady_clock::now();
	auto count = 10000;
	for (int i = 0; i < count; i++) {
		(void) test();
	}
	auto elapsed = std::chrono::steady_clock::now() - start;
	std::cout << "Mesh generation time: " <<
		(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / count) <<" us" << std::endl;
	return 0;
}
