cmake_minimum_required(VERSION 3.22)
project(cxx_benchmark)

set(CMAKE_CXX_STANDARD 20)

if(WIN32 AND NOT MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

add_subdirectory(lib/glm)

add_executable(cxx_benchmark src/main.cpp)
target_link_libraries(cxx_benchmark glm::glm)
