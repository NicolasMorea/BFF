#include "bff_scan.h"
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

bool is_bff_repo() {
    return fs::exists(".bff") && fs::is_directory(".bff");
}

std::string compute_hash(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

void scan_directory(
    const fs::path& root,
    const fs::path& current,
    std::unordered_map<std::string, std::vector<std::string>>& index_map
) {
    if (current != root && fs::exists(current / ".bff")) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(current)) {
        if (entry.path().filename() == ".bff") {
            continue;
        }

        if (entry.is_directory()) {
            scan_directory(root, entry.path(), index_map);
        }
        else if (entry.is_regular_file()) {
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) continue;

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();

            std::string hash = compute_hash(content);

            std::string rel_path = fs::relative(entry.path(), root).string();
            index_map[hash].push_back(rel_path);
        }
    }
}

void scan_current_with_nested(
    const fs::path& root,
    const fs::path& current,
    std::unordered_map<std::string, std::string>& path_hash
) {
    for (const auto& entry : fs::directory_iterator(current)) {
        if (entry.is_directory()) {
            if (fs::exists(entry.path() / ".bff")) {
                continue;
            } else {
                scan_current_with_nested(root, entry.path(), path_hash);
            }
        } else if (entry.is_regular_file()) {
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) continue;

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string hash = compute_hash(buffer.str());

            std::string rel_path = fs::relative(entry.path(), root).string();
            path_hash[rel_path] = hash;
        }
    }
}
