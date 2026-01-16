#include <iostream>
#include <string>
#include <cstring>
#include "bff_commands.h"

void printUsage() {
    std::cout << "Usage: bff <command> [<args>]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  init : Initialize a new bff repository\n";
    std::cout << "  index : Index the current directory tree\n";
    std::cout << "  clean : Clean the index file\n";
    std::cout << "  status : Show the status of the repository (diff)\n";
    std::cout << "  clones : Show cloned files\n";
    std::cout << "  compare <path> : Compare with another bff repository\n";
    std::cout << "  match <path> : Find files matching those in the given path\n";
}
    
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "init") {
        return bff_init();
    } else if (command == "index") {
        return bff_index();
    } else if (command == "clean") {
        return bff_clean();
    } else if (command == "status") {
        return bff_status();
    } else if (command == "compare") {
        if (argc < 3) {
            std::cerr << "Error: 'compare' requires a path argument\n";
            printUsage();
            return 1;
        }
        return bff_compare(argv[2]);
    } else if (command == "clones") {
        return bff_print_clones();
    } else if (command == "match") {
        if (argc < 3) {
            std::cerr << "Error: 'match' requires a path argument\n";
            printUsage();
            return 1;
        }
        return bff_match(argv[2]);
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        printUsage();
        return 1;
    }

    return 0;
}
