#pragma once

#include <string>
#include <vector>

struct TraversalResult {
    std::vector<std::string> cmake_files;   // all CMakeLists.txt found
    std::vector<std::string> source_files;  // all .c .cpp .h .hpp found
};

// Recursively traverses the given root directory
// skips symlinks and ignored directories
TraversalResult traverse_repo(const std::string& root_path);