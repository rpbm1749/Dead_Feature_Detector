#pragma once

#include <string>
#include <vector>
#include "../../include/types.hpp"

// Scans a single source file and extracts all macro references
// from #ifdef, #ifndef, #if defined(), #elif defined()
std::vector<SourceMacroRef> parse_source_file(const std::string& file_path);