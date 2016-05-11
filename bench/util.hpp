#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

std::string loadFile(const std::string& filename) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in) {
        std::ostringstream contents;
        contents << in.rdbuf();
        in.close();
        return contents.str();
    }
    throw std::runtime_error("Error opening file");
}

class Timer {
    using Clock = std::chrono::steady_clock;
    using SystemClock = std::chrono::system_clock;

    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;

    TimePoint start = Clock::now();

public:
    void report(const std::string& name) {
        Duration duration = Clock::now() - start;
        std::cerr << name << ": "
                  << double(
                         std::chrono::duration_cast<std::chrono::microseconds>(duration).count()) /
                         1000
                  << "ms" << std::endl;
        start += duration;
    }
};
