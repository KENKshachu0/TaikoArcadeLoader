#include "banner.h"

namespace Banner {

std::string
Version::getVer () {
    std::string fullVersion = std::string (TAL_VERSION);
    return fullVersion.substr (0, fullVersion.find("-"));
}

Version::Version () {
    // Print a cool logo with version info
    std::cout << R"(                                                                               )" << std::endl;
    std::cout << R"( ______     _ __        ___                   __    __                __       )" << std::endl;
    std::cout << R"(/_  __/__  (_) /_____  / _ | ___________  ___/ /__ / /  ___  ___  ___/ /__ ____)" << std::endl;
    std::cout << R"( / / / _ `/ /  '_/ _ \/ __ |/ __/ __/ _ `/ _  / -_) /__/ _ \/ _ `/ _  / -_) __/)" << std::endl;
    std::cout << R"(/_/  \_,_\_/_/\_\\___/_/ |_/_/  \__/\_,_\\_,_/\__/____/\___/\_,_\\_,_/\__/_/   )" << std::endl;
    std::cout << std::format("{: >79}", std::format("::v{}::", TAL_VERSION)) << std::endl;
    std::cout << R"(                                                                               )" << std::endl;
}

}