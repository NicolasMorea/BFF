#include "bff_data_load.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <openssl/sha.h>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

static void load_single_index(
    const fs::path& dir,
    const fs::path& root,
    std::unordered_map<std::string, std::string>& path_hash
) {
    fs::path index_file = dir / ".bff/index.json";
    if (!fs::exists(index_file)) return;

    std::ifstream file(index_file);
    if (!file) return;

    json j;
    file >> j;

    if (!j.contains("files")) return;

    for (auto& [hash, paths] : j["files"].items()) {
        for (auto& p : paths) {
            const fs::path absolute_path = dir / p.get<std::string>();
            std::string rel_path = fs::relative(absolute_path, root).string();
            path_hash[rel_path] = hash;
        }
    }
}

static void load_nested_indices(
    const fs::path& dir,
    const fs::path& root,
    std::unordered_map<std::string, std::string>& path_hash
) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) return;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_directory()) {
            if (fs::exists(entry.path() / ".bff")) {
                load_single_index(entry.path(), root, path_hash);
                load_nested_indices(entry.path(), root, path_hash);
            } else {
                load_nested_indices(entry.path(), root, path_hash);
            }
        }
    }
}

std::unordered_map<std::string, std::string> load_index(const fs::path& root) {

    std::unordered_map<std::string, std::string> path_hash;

    load_single_index(root, root, path_hash);
    load_nested_indices(root, root, path_hash);

    return path_hash;
}

void write_index_json(const std::unordered_map<std::string, std::vector<std::string>>& index_map) {

    fs::create_directories(".bff");

    std::ofstream out(".bff/index.json");
    if (!out) {
        throw std::runtime_error("Could not write .bff/index.json");
    }
    out << "{\n  \"files\": {\n";

    bool first_hash = true;
    for (const auto& [hash, paths] : index_map) {
        if (!first_hash) out << ",\n";
        first_hash = false;

        out << "    \"" << hash << "\": [";

        for (size_t i = 0; i < paths.size(); ++i) {
            out << "\"" << paths[i] << "\"";
            if (i + 1 < paths.size()) out << ", ";
        }

        out << "]";
    }

    out << "\n  }\n}\n";
}


