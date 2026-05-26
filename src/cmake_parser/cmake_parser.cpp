#include "cmake_parser.hpp"
#include "add_definitions.hpp"
#include "variable_parser.hpp"
#include "target_parser.hpp"

#include <fstream>
#include <iostream>
#include <regex>
#include <unordered_map>

// ─────────────────────────────────────────────
//  Call type routing
// ─────────────────────────────────────────────

enum class CallType {
    AddDefinitions,
    Set,
    List,
    Option,
    TargetDefinitions,
    Unknown,
    AddLibrary,
    AddExecutable
};

static const std::unordered_map<std::string, CallType> keyword_map = {
    { "add_definitions",            CallType::AddDefinitions },
    { "add_compile_definitions",    CallType::AddDefinitions },
    { "set",                        CallType::Set            },
    { "list",                       CallType::List           },
    { "option",                     CallType::Option         },
    { "target_compile_definitions", CallType::TargetDefinitions },
    { "add_library",                CallType::AddLibrary        },
    { "add_executable",             CallType::AddExecutable     }
};

// ─────────────────────────────────────────────
//  Shared utilities
// ─────────────────────────────────────────────

static std::string strip_comment(const std::string& line) {
    auto pos = line.find('#');
    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}

// Tries to match a cmake keyword call at the start of a line
// Returns the keyword and the content after the opening paren
// e.g. "  add_definitions( -DFOO" → {"add_definitions", "-DFOO"}
static std::pair<std::string, std::string> match_call_start(const std::string& line) {
    static const std::regex call_regex(
        R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\((.*))",
        std::regex::icase
    );

    std::smatch match;
    if (std::regex_search(line, match, call_regex)) {
        return { match[1].str(), match[2].str() };
    }

    return { "", "" };
}

// Accumulates lines until the call's parentheses are balanced
// Returns the full args string (content between the outermost parens)
// i is advanced to the line containing the closing paren
static std::string accumulate_block(
    const std::vector<std::string>& lines,
    int& i,
    const std::string& first_fragment
) {
    std::string accumulated = first_fragment;
    int depth = 1;

    // count depth from the first fragment
    for (char c : accumulated) {
        if      (c == '(') ++depth;
        else if (c == ')') --depth;
    }

    // already closed on the same line
    if (depth == 0) {
        auto close = accumulated.rfind(')');
        if (close != std::string::npos) {
            accumulated = accumulated.substr(0, close);
        }
        return accumulated;
    }

    // keep reading until balanced
    const int total = static_cast<int>(lines.size());
    while (depth > 0 && ++i < total) {
        std::string next = strip_comment(lines[i]);

        for (char c : next) {
            if (c == '(') ++depth;
            else if (c == ')') {
                --depth;
                if (depth == 0) break;
            }
        }

        if (depth == 0) {
            auto close = next.find(')');
            if (close != std::string::npos) {
                accumulated += " " + next.substr(0, close);
            }
        } else {
            accumulated += " " + next;
        }
    }

    return accumulated;
}

// ─────────────────────────────────────────────
//  Main entry point
// ─────────────────────────────────────────────

void parse_cmake_file(const std::string& file_path, ProjectData& data) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[warn] could not open: " << file_path << "\n";
        return;
    }

    // read all lines upfront
    std::vector<std::string> lines;
    {
        std::string line_str;
        while (std::getline(file, line_str)) {
            lines.push_back(line_str);
        }
    }

    const int total = static_cast<int>(lines.size());
    int i = 0;

    while (i < total) {
        std::string cleaned = strip_comment(lines[i]);
        auto [keyword, first_fragment] = match_call_start(cleaned);

        if (keyword.empty()) {
            ++i;
            continue;
        }

        // lowercase the keyword for map lookup
        std::string keyword_lower = keyword;
        for (auto& c : keyword_lower) c = std::tolower(c);

        auto it = keyword_map.find(keyword_lower);
        if (it == keyword_map.end()) {
            ++i;
            continue;
        }

        CallType type    = it->second;
        int start_line   = i + 1;   // 1-indexed
        std::string args = accumulate_block(lines, i, first_fragment);

        // route to the right processor
        switch (type) {
            case CallType::AddDefinitions: {
                auto defs = process_add_definitions(args, file_path, start_line);
                data.cmake_definitions.insert(
                    data.cmake_definitions.end(),
                    defs.begin(), defs.end()
                );
                break;
            }

            case CallType::Set:
                process_variable_set(args, file_path, start_line, data.variable_map);
                break;

            case CallType::List:
                process_variable_list(args, file_path, start_line, data.variable_map);
                break;

            case CallType::Option:
                process_option(args, file_path, start_line, data.variable_map);
                break;

            case CallType::Unknown:
                break;

            case CallType::AddLibrary:
                process_add_library(args, file_path, start_line, data.target_map);
                break;

            case CallType::AddExecutable:
                process_add_executable(args, file_path, start_line, data.target_map);
                break;

            case CallType::TargetDefinitions:
                process_target_compile_definitions(
                    args,
                    file_path,
                    start_line,
                    data.target_map,
                    data.target_definitions,
                    data.unknown_target_refs
                );
                break;
        }

        ++i;
    }
}