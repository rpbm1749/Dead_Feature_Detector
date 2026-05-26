#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "../../include/types.hpp"

void process_add_library(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeTarget>& target_map
);

void process_add_executable(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeTarget>& target_map
);

void process_target_compile_definitions(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    const std::unordered_map<std::string, CMakeTarget>& target_map,
    std::vector<CMakeTargetDefinition>& target_definitions,
    std::vector<UnknownTargetRef>& unknown_target_refs
);