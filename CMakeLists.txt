cmake_minimum_required(VERSION 3.16)
project(dead_feature_detector CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dfd
    src/main.cpp
    src/traversal/traversal.cpp
    src/cmake_parser/cmake_parser.cpp
    src/cmake_parser/add_definitions.cpp
    src/cmake_parser/variable_parser.cpp
    src/cmake_parser/target_parser.cpp
    src/source_parser/source_parser.cpp
    src/resolver/resolver.cpp
)

target_include_directories(dfd PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)