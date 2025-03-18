#include <string>
#include <iostream>

#include "config.h"


int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config file>" << std::endl;
        return 1;
    }

    std::string fullPath = argv[1];
    if (!Config::ConfigManager::validateConfigfile(fullPath)) {
        std::cerr << "Config file is invalid" << std::endl;
        return 1;
    }

    std::cout << "Config file is valid" << std::endl;
    return 0;
}