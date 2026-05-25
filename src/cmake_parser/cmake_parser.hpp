#pragma once

#include <string>
#include "../../include/types.hpp"

// Parses a single CMakeLists.txt file and populates ProjectData
void parse_cmake_file(const std::string& file_path, ProjectData& data);