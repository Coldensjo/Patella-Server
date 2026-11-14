cmake_minimum_required(VERSION 3.20)
project(patella CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra)

enable_testing()

add_subdirectory(extern/tomlplusplus)
add_subdirectory(keygen)
add_subdirectory(querymanager)
add_subdirectory(loginserver)
add_subdirectory(src)
add_subdirectory(tools/convert)
