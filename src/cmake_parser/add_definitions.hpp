#pragma once

#include <string>
#include <vector>
#include "../../include/types.hpp"

// Processes an already-extracted add_definitions / add_compile_definitions
// args string and returns the resulting definitions
std::vector<CMakeDefinition> process_add_definitions(
    const std::string& args,
    const std::string& file_path,
    int start_line
);