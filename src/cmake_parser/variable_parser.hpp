#pragma once

#include <string>
#include <unordered_map>
#include "../../include/types.hpp"

void process_variable_set(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeVariable>& variable_map
);

void process_variable_list(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeVariable>& variable_map
);

void process_option(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeVariable>& variable_map
);