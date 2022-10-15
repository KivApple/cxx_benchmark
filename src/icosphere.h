#pragma once

#include <utility>
#include <vector>
#include <glm/vec3.hpp>

std::pair<std::vector<glm::vec3>, std::vector<glm::vec<3, unsigned int>>> generateMesh(int subdivisionCount);
