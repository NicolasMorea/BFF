#include "bff_commands.h"
#include "bff_data_load.h"
#include "bff_scan.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <openssl/sha.h>
#include <algorithm>
#include <unordered_set>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace fs = std::filesystem;

int bff_init() {
    if (is_bff_repo()) {
        std::cout << "Reinitialized existing bff repository in " << fs::current_path() << "/.bff/\n";
        return 0;
    }

    try {
        fs::create_directory(".bff");
        
        std::cout << "Initialized empty bff repository in " << fs::current_path() << "/.bff/\n";
        return 0;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error initializing repository: " << e.what() << "\n";
        return 1;
    }
}

static void index_nested_repos(const fs::path& current) {
    if (!fs::exists(current) || !fs::is_directory(current)) return;

    for (const auto& entry : fs::directory_iterator(current)) {
        if (entry.is_directory()) {
            if (fs::exists(entry.path() / ".bff")) {
                auto original_path = fs::current_path();
                fs::current_path(entry.path());
                
                std::unordered_map<std::string, std::vector<std::string>> nested_index_map;
                scan_directory(entry.path(), entry.path(), nested_index_map);
                write_index_json(nested_index_map);
                
                std::cout << "  Indexed nested repository: " << fs::relative(entry.path(), original_path).string() << " (" << nested_index_map.size() << " hashes)\n";
                
                index_nested_repos(entry.path());
                
                fs::current_path(original_path);
            } else {
                index_nested_repos(entry.path());
            }
        }
    }
}

int bff_index() {
    if (!is_bff_repo()) {
        std::cerr << "Error: Not a bff repository\n";
        return 1;
    }

    try {
        std::unordered_map<std::string, std::vector<std::string>> index_map;

        fs::path root = fs::current_path();
        scan_directory(root, root, index_map);

        write_index_json(index_map);

        std::cout << "Indexed files into .bff/index.json" << std::endl;
        std::cout << "Tracked " << index_map.size() << " unique file hashes" << std::endl;

        index_nested_repos(root);

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Indexing failed: " << e.what() << std::endl;
        return 1;
    }
}

int bff_clean()
{
    if (!is_bff_repo()) {
        std::cerr << "Error: Not a bff repository" << std::endl;
        return 1;
    }

    try {
        fs::path index_file = ".bff/index.json";
        if (fs::exists(index_file)) {
            fs::remove(index_file);
            std::cout << "Cleaned index file: " << index_file << std::endl;
        } else {
            std::cout << "No index file to clean" << std::endl;
        }
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Cleaning failed: " << e.what() << std::endl;
        return 1;
    }
}

int bff_status() {
    if (!is_bff_repo()) {
        std::cerr << "Not a bff repository" << std::endl;
        return 1;
    }

    auto index_path_hash = load_index(fs::current_path());

    std::unordered_map<std::string, std::string> current_path_hash;
    fs::path root = fs::current_path();
    scan_current_with_nested(root, root, current_path_hash);

    std::vector<std::string> modified, deleted, added;

    for (auto& [path, old_hash] : index_path_hash) {
        auto it = current_path_hash.find(path);
        if (it == current_path_hash.end())
            deleted.push_back(path);
        else if (it->second != old_hash)
            modified.push_back(path);
    }

    for (auto& [path, hash] : current_path_hash) {
        if (index_path_hash.find(path) == index_path_hash.end())
            added.push_back(path);
    }

    if (added.empty() && modified.empty() && deleted.empty()) {
        std::cout << "No changes" << std::endl;
        return 0;
    }

    if (!modified.empty()) {
        std::cout << "Modified files:" << std::endl;
        for (auto& p : modified) std::cout << "  " << p << std::endl;
    }

    if (!deleted.empty()) {
        std::cout << "Deleted files:" << std::endl;
        for (auto& p : deleted) std::cout << "  " << p << std::endl;
    }

    if (!added.empty()) {
        std::cout << "New files:" << std::endl;
        for (auto& p : added) std::cout << "  " << p << std::endl;
    }

    return 0;
}

int bff_print_clones() {
    auto index_map = load_index(fs::current_path());

    std::unordered_map<std::string, std::vector<std::string>> hash_to_paths;
    for (const auto& [path, hash] : index_map) {
        hash_to_paths[hash].push_back(path);
    }

    for (const auto& [hash, paths] : hash_to_paths) {
        if (paths.size() > 1) {
            std::cout << "Hash: " << hash << std::endl;
            for (const auto& p : paths) {
                std::cout << "  " << p << std::endl;
            }
        }
    }

    return 0;
}

int bff_compare(const std::string& path) {
    auto index_this = load_index(fs::current_path());
    auto index_other = load_index(fs::path(path));

    int this_not_in_other_by_path = 0;
    int other_not_in_this_by_path = 0;
    int same_path_same_content = 0;

    for (const auto& [p, h] : index_this) {
        auto it = index_other.find(p);
        if (it == index_other.end()) {
            this_not_in_other_by_path++;
        } else if (it->second == h) {
            same_path_same_content++;
        }
    }
    for (const auto& [p, h] : index_other) {
        if (index_this.find(p) == index_this.end()) {
            other_not_in_this_by_path++;
        }
    }

    std::unordered_set<std::string> this_hashes;
    this_hashes.reserve(index_this.size());
    for (const auto& [p, h] : index_this) this_hashes.insert(h);

    std::unordered_set<std::string> other_hashes;
    other_hashes.reserve(index_other.size());
    for (const auto& [p, h] : index_other) other_hashes.insert(h);

    int this_missing_in_other_by_content = 0;
    for (const auto& h : this_hashes) {
        if (other_hashes.find(h) == other_hashes.end()) this_missing_in_other_by_content++;
    }
    int other_missing_in_this_by_content = 0;
    for (const auto& h : other_hashes) {
        if (this_hashes.find(h) == this_hashes.end()) other_missing_in_this_by_content++;
    }
    int common_contents = 0;
    for (const auto& h : this_hashes) {
        if (other_hashes.find(h) != other_hashes.end()) common_contents++;
    }

    std::cout << "By path (same relative path):" << std::endl;
    std::cout << "  Files in this not in other: " << this_not_in_other_by_path << std::endl;
    std::cout << "  Files in other not in this: " << other_not_in_this_by_path << std::endl;
    std::cout << "  Same path + same content: " << same_path_same_content << std::endl;

    std::cout << "By content (hash overlap):" << std::endl;
    std::cout << "  Unique contents in this not in other: " << this_missing_in_other_by_content << std::endl;
    std::cout << "  Unique contents in other not in this: " << other_missing_in_this_by_content << std::endl;
    std::cout << "  Common contents (unique hashes): " << common_contents << std::endl;
    return 0;
}

int bff_match(const std::string& path) {
    const fs::path this_root = fs::current_path();
    const fs::path source_root = fs::path(path);

    auto index_this = load_index(this_root);
    auto index_source = load_index(source_root);

    std::unordered_map<std::string, std::vector<std::string>> hash_to_this_paths;
    std::unordered_set<std::string> this_hashes;
    this_hashes.reserve(index_this.size());
    for (const auto& [this_path, this_hash] : index_this) {
        hash_to_this_paths[this_hash].push_back(this_path);
        this_hashes.insert(this_hash);
    }

    std::unordered_map<std::string, std::unordered_map<std::string, int>> dir_votes;
    for (const auto& [source_path, source_hash] : index_source) {
        auto it = hash_to_this_paths.find(source_hash);
        if (it == hash_to_this_paths.end() || it->second.empty()) continue;

        const std::string source_dir = fs::path(source_path).parent_path().string();
        const std::string dest_dir = fs::path(it->second.front()).parent_path().string();
        dir_votes[source_dir][dest_dir] += 1;
    }

    std::unordered_map<std::string, std::string> source_dir_to_dest_dir;
    for (const auto& [source_dir, candidates] : dir_votes) {
        int best_votes = -1;
        std::string best_dest_dir;
        for (const auto& [dest_dir, votes] : candidates) {
            if (votes > best_votes || (votes == best_votes && dest_dir < best_dest_dir)) {
                best_votes = votes;
                best_dest_dir = dest_dir;
            }
        }
        if (!best_dest_dir.empty()) {
            source_dir_to_dest_dir[source_dir] = best_dest_dir;
        }
    }

    int copied_files = 0;
    std::unordered_set<std::string> copied_hashes;
    copied_hashes.reserve(index_source.size());

    for (const auto& [source_rel_str, source_hash] : index_source) {
        if (this_hashes.find(source_hash) != this_hashes.end()) continue;
        if (copied_hashes.find(source_hash) != copied_hashes.end()) continue;

        const fs::path source_rel = fs::path(source_rel_str);
        const fs::path source_abs = source_root / source_rel;

        fs::path target_dir_rel = source_rel.parent_path();
        auto map_it = source_dir_to_dest_dir.find(source_rel.parent_path().string());
        if (map_it != source_dir_to_dest_dir.end()) {
            target_dir_rel = fs::path(map_it->second);
        }

        const fs::path target_abs = this_root / target_dir_rel / source_rel.filename();

        try {
            fs::create_directories(target_abs.parent_path());
            fs::copy_file(source_abs, target_abs, fs::copy_options::overwrite_existing);
            copied_files++;
            copied_hashes.insert(source_hash);
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error copying " << source_abs << " -> " << target_abs << ": " << e.what() << std::endl;
        }
    }

    std::cout << "Copied " << copied_files << " files" << std::endl;
    return 0;
}