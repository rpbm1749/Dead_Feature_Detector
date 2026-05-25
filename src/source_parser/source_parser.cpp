#include "source_parser.hpp"

#include <fstream>
#include <regex>
#include <iostream>
#include <stack>

// ─────────────────────────────────────────────
//  Utilities
// ─────────────────────────────────────────────

// Strips single-line comments from a line
// Does not handle block comments /* */ — deferred
static std::string strip_line_comment(const std::string& line) {
    auto pos = line.find("//");
    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}

// Extracts all macro names from a #if defined() / #elif defined() expression
// Handles:
//   defined(FOO)
//   defined(FOO) && defined(BAR)
//   defined(FOO) || defined(BAR)
//   !defined(FOO)
// Returns pairs of {macro_name, is_negated}
static std::vector<std::pair<std::string, bool>> extract_defined_macros(
    const std::string& expr
) {
    std::vector<std::pair<std::string, bool>> results;

    // matches optional ! before defined(MACRO)
    static const std::regex defined_regex(
        R"((!?)\bdefined\s*\(\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\))"
    );

    auto begin = std::sregex_iterator(expr.begin(), expr.end(), defined_regex);
    auto end   = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        const std::smatch& m = *it;
        bool is_negated      = !m[1].str().empty();  // true if ! precedes defined
        std::string name     = m[2].str();
        results.push_back({ name, is_negated });
    }

    return results;
}

// ─────────────────────────────────────────────
//  Directive matching
// ─────────────────────────────────────────────

struct ParsedDirective {
    enum class Type {
        Ifdef,
        Ifndef,
        If,
        Elif,
        Endif,
        Other
    };

    Type        type;
    std::string raw_expr;   // everything after the directive keyword
};

static ParsedDirective parse_directive(const std::string& line) {
    static const std::regex directive_regex(
        R"(^\s*#\s*(ifdef|ifndef|if|elif|endif)\b(.*))",
        std::regex::icase
    );

    std::smatch match;
    if (!std::regex_search(line, match, directive_regex)) {
        return { ParsedDirective::Type::Other, "" };
    }

    std::string keyword = match[1].str();
    std::string expr    = match[2].str();

    // lowercase keyword for comparison
    for (auto& c : keyword) c = std::tolower(c);

    if      (keyword == "ifdef")  return { ParsedDirective::Type::Ifdef,  expr };
    else if (keyword == "ifndef") return { ParsedDirective::Type::Ifndef, expr };
    else if (keyword == "if")     return { ParsedDirective::Type::If,     expr };
    else if (keyword == "elif")   return { ParsedDirective::Type::Elif,   expr };
    else if (keyword == "endif")  return { ParsedDirective::Type::Endif,  "" };

    return { ParsedDirective::Type::Other, "" };
}

// ─────────────────────────────────────────────
//  Main entry point
// ─────────────────────────────────────────────

std::vector<SourceMacroRef> parse_source_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[warn] could not open: " << file_path << "\n";
        return {};
    }

    std::vector<SourceMacroRef> results;

    // stack tracks open ifdef blocks
    // each entry is { start_line, vector of SourceMacroRef indices in results }
    // when we hit #endif we know the span and patch it in
    struct OpenBlock {
        int start_line;
        std::vector<size_t> ref_indices;    // indices into results for this block
    };
    std::stack<OpenBlock> block_stack;

    std::vector<std::string> lines;
    {
        std::string line_str;
        while (std::getline(file, line_str)) {
            lines.push_back(line_str);
        }
    }

    const int total = static_cast<int>(lines.size());

    for (int i = 0; i < total; ++i) {
        std::string cleaned = strip_line_comment(lines[i]);
        int line_num        = i + 1;    // 1-indexed

        ParsedDirective directive = parse_directive(cleaned);

        switch (directive.type) {

            case ParsedDirective::Type::Ifdef: {
                // #ifdef FOO — single macro, trim whitespace
                std::string macro = directive.raw_expr;
                // trim leading/trailing whitespace
                auto start = macro.find_first_not_of(" \t");
                auto end   = macro.find_last_not_of(" \t\r\n");
                if (start == std::string::npos) break;
                macro = macro.substr(start, end - start + 1);

                SourceMacroRef ref;
                ref.macro_name  = macro;
                ref.file_path   = file_path;
                ref.line        = line_num;
                ref.span_lines  = 0;        // patched when #endif is found
                ref.is_negated  = false;

                size_t idx = results.size();
                results.push_back(ref);

                block_stack.push({ line_num, { idx } });
                break;
            }

            case ParsedDirective::Type::Ifndef: {
                // #ifndef FOO — single macro, is_negated = true
                std::string macro = directive.raw_expr;
                auto start = macro.find_first_not_of(" \t");
                auto end   = macro.find_last_not_of(" \t\r\n");
                if (start == std::string::npos) break;
                macro = macro.substr(start, end - start + 1);

                SourceMacroRef ref;
                ref.macro_name  = macro;
                ref.file_path   = file_path;
                ref.line        = line_num;
                ref.span_lines  = 0;
                ref.is_negated  = true;

                size_t idx = results.size();
                results.push_back(ref);

                block_stack.push({ line_num, { idx } });
                break;
            }

            case ParsedDirective::Type::If:
            case ParsedDirective::Type::Elif: {
                // #if defined(FOO) && defined(BAR) etc.
                // extract all macros from the expression
                auto macros = extract_defined_macros(directive.raw_expr);
                if (macros.empty()) {
                    // #if with no defined() calls — still need to track
                    // the block for depth purposes but no refs to record
                    if (directive.type == ParsedDirective::Type::If) {
                        block_stack.push({ line_num, {} });
                    }
                    break;
                }

                OpenBlock block{ line_num, {} };

                for (auto& [name, is_negated] : macros) {
                    SourceMacroRef ref;
                    ref.macro_name  = name;
                    ref.file_path   = file_path;
                    ref.line        = line_num;
                    ref.span_lines  = 0;
                    ref.is_negated  = is_negated;

                    size_t idx = results.size();
                    results.push_back(ref);
                    block.ref_indices.push_back(idx);
                }

                // only push a new block frame for #if, not #elif
                // #elif belongs to the same depth level
                if (directive.type == ParsedDirective::Type::If) {
                    block_stack.push(block);
                } else {
                    // for #elif, patch the existing open block's refs
                    // and add the new refs to the current frame
                    if (!block_stack.empty()) {
                        for (auto idx : block.ref_indices) {
                            block_stack.top().ref_indices.push_back(idx);
                        }
                    }
                }
                break;
            }

            case ParsedDirective::Type::Endif: {
                if (block_stack.empty()) break;

                OpenBlock block = block_stack.top();
                block_stack.pop();

                int span = line_num - block.start_line + 1;

                // patch span_lines for all refs belonging to this block
                for (auto idx : block.ref_indices) {
                    results[idx].span_lines = span;
                }
                break;
            }

            case ParsedDirective::Type::Other:
                break;
        }
    }

    // handle unclosed blocks — file ended before #endif
    // patch whatever span we can
    while (!block_stack.empty()) {
        OpenBlock block = block_stack.top();
        block_stack.pop();

        int span = total - block.start_line + 1;
        for (auto idx : block.ref_indices) {
            results[idx].span_lines = span;
        }
    }

    return results;
}