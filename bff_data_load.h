#ifndef BFF_DATA_LOAD_H
#define BFF_DATA_LOAD_H

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

std::unordered_map<std::string, std::string> load_index(const fs::path& root);

void write_index_json(const std::unordered_map<std::string, std::vector<std::string>>& index_map);

#endif