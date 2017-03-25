#pragma once
#include <vector>

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

