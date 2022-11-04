cmake_minimum_required(VERSION 3.2)
project(cxx_benchmark)

set(CMAKE_CXX_STANDARD 20)

if(WIN32 AND NOT MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

set(BUILD_SHARED_LIBS OFF)
set(ABSL_PROPAGATE_CXX_STD ON)

add_subdirectory(lib/glm)
add_subdirectory(lib/bitsery)
add_subdirectory(lib/abseil-cpp)

add_executable(cxx_benchmark src/main.cpp src/icosphere.cpp src/pf2_font_loader.cpp)
target_link_libraries(cxx_benchmark glm::glm Bitsery::bitsery absl::flat_hash_map)

configure_file(assets/DroidSans-32.pf2 assets/DroidSans-32.pf2 COPYONLY)
