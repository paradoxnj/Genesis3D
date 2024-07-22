cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(GENESIS_INCLUDE_PATH ${CMAKE_CURRENT_LIST_DIR}/include)
set(GENESIS_DEPS_PATH ${CMAKE_CURRENT_LIST_DIR}/dependencies)

project(GEN3D C CXX ASM)

add_subdirectory(source)
