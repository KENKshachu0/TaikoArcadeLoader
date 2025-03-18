#pragma once
#include <string>
#include <format>
#include <iostream>

namespace Banner {
class Version {
public:

    std::string getVer();

    // Singleton pattern for global access
    static Version& instance() {
        static Version instance;  // Guaranteed to be destroyed, instantiated on first use
        return instance;
    }

private:

    Version();
    ~Version() = default;

};

}