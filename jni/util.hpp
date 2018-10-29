#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

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

/**
 * L1 measurement of distance between points
 */
int16_t measure(const mapbox::geometry::line_string<int16_t>& lineString) {
  int16_t distance = 0;
  for (unsigned long j=1; j<lineString.size(); j++) {
    distance += std::abs(lineString[j].x - lineString[j-1].x);
    distance += std::abs(lineString[j].y - lineString[j-1].y);
  }
  return distance;
}
