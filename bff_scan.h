#ifndef BFF_SCAN_H
#define BFF_SCAN_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <openssl/sha.h>
#include <algorithm>

namespace fs = std::filesystem;

std::string compute_hash(const std::string& data);

bool is_bff_repo();

void scan_directory(
    const fs::path& root,
    const fs::path& current,
    std::unordered_map<std::string, std::vector<std::string>>& index_map
);

void scan_current_with_nested(
    const fs::path& root,
    const fs::path& current,
    std::unordered_map<std::string, std::string>& path_hash
);

#endif
