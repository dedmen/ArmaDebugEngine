#pragma once
#include <vector>
#include <string>

class Tracker {
public:
    Tracker();
    ~Tracker();
    struct trackerCustomVariable {
        unsigned int customVarID;
        std::string name;
        std::string value;
    };
    static void trackPiwik();
};

