#include "traversal.hpp"

#include <filesystem>
#include <iostream>
#include <unordered_set>
#include <algorithm>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────
//  Hardcoded ignore list
// ─────────────────────────────────────────────

static const std::unordered_set<std::string> IGNORED_DIRS = {
    "build",
    "builds",
    ".git",
    "CMakeFiles",
    "third_party",
    "vendor",
    "extern",
    "deps",
    "external",
    ".cache",
    "__pycache__",
};

// ─────────────────────────────────────────────
//  Hardcoded source extensions
// ─────────────────────────────────────────────

static const std::unordered_set<std::string> SOURCE_EXTENSIONS = {
    ".c",
    ".cpp",
    ".h",
    ".hpp",
};

// ─────────────────────────────────────────────
//  Utilities
// ─────────────────────────────────────────────

static bool is_ignored_dir(const fs::path& path) {
    return IGNORED_DIRS.count(path.filename().string()) > 0;
}

static bool is_source_file(const fs::path& path) {
    return SOURCE_EXTENSIONS.count(path.extension().string()) > 0;
}

static bool is_cmake_file(const fs::path& path) {
    return path.filename() == "CMakeLists.txt";
}

// ─────────────────────────────────────────────
//  Main entry point
// ─────────────────────────────────────────────

TraversalResult traverse_repo(const std::string& root_path) {
    TraversalResult result;

    fs::path root(root_path);

    if (!fs::exists(root)) {
        std::cerr << "[error] path does not exist: " << root_path << "\n";
        return result;
    }

    if (!fs::is_directory(root)) {
        std::cerr << "[error] path is not a directory: " << root_path << "\n";
        return result;
    }

    // recursive_directory_iterator with skip_permission_denied
    // so we don't crash on protected directories
    fs::recursive_directory_iterator it(
        root,
        fs::directory_options::skip_permission_denied
    );
    fs::recursive_directory_iterator end;

    while (it != end) {
        const fs::directory_entry& entry = *it;
        const fs::path& path             = entry.path();

        // skip symlinks entirely
        if (fs::is_symlink(path)) {
            it.disable_recursion_pending();
            ++it;
            continue;
        }

        // if it's a directory, check if it should be ignored
        if (fs::is_directory(path)) {
            if (is_ignored_dir(path)) {
                it.disable_recursion_pending();
                ++it;
                continue;
            }
            ++it;
            continue;
        }

        // it's a file — check what kind
        if (fs::is_regular_file(path)) {
            if (is_cmake_file(path)) {
                result.cmake_files.push_back(path.string());
            } else if (is_source_file(path)) {
                result.source_files.push_back(path.string());
            }
        }

        ++it;
    }

    // sort both lists for deterministic output
    std::sort(result.cmake_files.begin(),  result.cmake_files.end());
    std::sort(result.source_files.begin(), result.source_files.end());

    return result;
}