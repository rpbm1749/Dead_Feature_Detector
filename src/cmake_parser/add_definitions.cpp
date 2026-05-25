#include "add_definitions.hpp"
#include <sstream>

static std::pair<std::string, std::optional<std::string>> strip_and_split(const std::string& token) {
    std::string clean = token;

    if (clean.size() >= 2 && clean[0] == '-' && clean[1] == 'D') {
        clean = clean.substr(2);
    }

    auto eq = clean.find('=');
    if (eq != std::string::npos) {
        return { clean.substr(0, eq), clean.substr(eq + 1) };
    }

    return { clean, std::nullopt };
}

static std::vector<std::string> tokenize_args(const std::string& args_str) {
    std::vector<std::string> tokens;
    std::istringstream ss(args_str);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<CMakeDefinition> process_add_definitions(
    const std::string& args,
    const std::string& file_path,
    int start_line
) {
    std::vector<CMakeDefinition> results;
    auto tokens = tokenize_args(args);

    for (const auto& token : tokens) {
        if (token.empty()) continue;

        auto [macro_name, macro_value] = strip_and_split(token);
        if (macro_name.empty()) continue;

        CMakeDefinition def;
        def.macro_name       = macro_name;
        def.value            = macro_value;
        def.file_path        = file_path;
        def.line             = start_line;
        def.needs_resolution = macro_name.find("${") != std::string::npos;

        results.push_back(def);
    }

    return results;
}