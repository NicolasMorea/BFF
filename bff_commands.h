#ifndef BFF_COMMANDS_H
#define BFF_COMMANDS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iomanip>
#include <unordered_map>
#include <vector>
#include <openssl/sha.h>
#include <algorithm>

// Create `.bff/` in the current directory.
int bff_init();

// Scan the directory tree, compute content hashes, and write `.bff/index.json`.
int bff_index();

// Remove `.bff/index.json`.
int bff_clean();

// Compare current files vs the saved index and report `Modified`, `Deleted`, and `New` files.
int bff_status();

// Print groups of files that share the same content hash (duplicate contents).
int bff_print_clones();

// Compare this repo’s index to another repo’s index, reporting differences by relative path and by content overlap.
int bff_compare(const std::string& path);

// Copy files from `<path>` into the current repo when their content hash is missing here.
// Attempts to map source folders to “best matching” destination folders using hash-based matching.
int bff_match(const std::string& path);

#endif
