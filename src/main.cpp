#include <iostream>
#include <string>

#include "traversal/traversal.hpp"
#include "cmake_parser/cmake_parser.hpp"
#include "source_parser/source_parser.hpp"
#include "../include/types.hpp"

// Temporary debug dump — will be replaced by the reporter later
static void dump_project_data(const ProjectData& data) {
    std::cout << "\n=== CMAKE DEFINITIONS ===\n";
    for (const auto& def : data.cmake_definitions) {
        std::cout << "  [" << def.file_path << ":" << def.line << "] "
                  << def.macro_name;
        if (def.value) std::cout << " = " << *def.value;
        if (def.needs_resolution) std::cout << " [UNRESOLVED]";
        std::cout << "\n";
    }

    std::cout << "\n=== CMAKE VARIABLES ===\n";
    for (const auto& [name, var] : data.variable_map) {
        std::cout << "  " << name;
        if (var.is_option) std::cout << " [OPTION]";
        if (var.is_cache)  std::cout << " [CACHE]";
        std::cout << "\n";
        for (const auto& val : var.possible_values) {
            std::cout << "    -> " << val.raw_value
                      << " (" << val.file_path << ":" << val.line << ")\n";
        }
    }

    std::cout << "\n=== SOURCE MACRO REFS ===\n";
    for (const auto& ref : data.source_refs) {
        std::cout << "  [" << ref.file_path << ":" << ref.line << "] "
                  << "#" << (ref.is_negated ? "ifndef" : "ifdef") << " "
                  << ref.macro_name
                  << " (span: " << ref.span_lines << " lines)\n";
    }

    std::cout << "\n=== TARGET DEFINITIONS ===\n";
    for (const auto& def : data.target_definitions) {
        std::cout << "  [" << def.file_path << ":" << def.line << "] "
                << def.target_name << " (" << def.visibility << ") "
                << def.macro_name;
        if (def.value)            std::cout << " = " << *def.value;
        if (def.needs_resolution) std::cout << " [UNRESOLVED]";
        std::cout << "\n";
    }

    std::cout << "\n=== UNKNOWN TARGETS ===\n";
    for (const auto& ref : data.unknown_target_refs) {
        std::cout << "  [" << ref.file_path << ":" << ref.line << "] "
                << ref.target_name << "\n";
    }

    std::cout << "\n=== KNOWN TARGETS ===\n";
    for (const auto& [name, target] : data.target_map) {
        std::cout << "  " << name << " (";
        switch (target.type) {
            case TargetType::Executable:        std::cout << "executable"; break;
            case TargetType::Library:           std::cout << "library";    break;
            case TargetType::InterfaceLibrary:  std::cout << "interface";  break;
            case TargetType::Unknown:           std::cout << "unknown";    break;
        }
        std::cout << ") [" << target.file_path << ":" << target.line << "]\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: dfd <repo_path>\n";
        return 1;
    }

    std::string repo_path = argv[1];

    std::cout << "[*] traversing: " << repo_path << "\n";
    TraversalResult traversal = traverse_repo(repo_path);

    std::cout << "[*] found " << traversal.cmake_files.size()  << " cmake files\n";
    std::cout << "[*] found " << traversal.source_files.size() << " source files\n";

    ProjectData data;

    // parse all cmake files
    std::cout << "[*] parsing cmake files...\n";
    for (const auto& path : traversal.cmake_files) {
        parse_cmake_file(path, data);
    }

    // parse all source files
    std::cout << "[*] parsing source files...\n";
    for (const auto& path : traversal.source_files) {
        auto refs = parse_source_file(path);
        data.source_refs.insert(
            data.source_refs.end(),
            refs.begin(), refs.end()
        );
    }

    // dump everything for now
    dump_project_data(data);

    return 0;
}