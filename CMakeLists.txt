cmake_minimum_required(VERSION 3.14)
project(nyc311 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
find_package(OpenMP REQUIRED)

add_subdirectory(src/phase1)
add_subdirectory(src/phase2)
add_subdirectory(src/phase3)
