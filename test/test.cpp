#include <mapbox/geojson.hpp>
#include <mapbox/geojsonvt.hpp>

#include <iostream>

class Timer {
public:
    std::chrono::high_resolution_clock::time_point started;
    Timer() {
        started = std::chrono::high_resolution_clock::now();
    }
    void operator()(std::string msg) {
        const auto now = std::chrono::high_resolution_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now - started);
        std::cerr << msg << ": " << double(ms.count()) / 1000 << "ms\n";
        started = now;
    }
};

char* readFile(const char* path) {
    std::FILE* f = std::fopen(path, "r");
    if (f == nullptr)
        throw std::runtime_error("Error opening file " + std::string(path));

    std::fseek(f, 0, SEEK_END);
    size_t size = std::ftell(f);
    char* str = new char[size];
    std::rewind(f);

    size_t result = std::fread(str, sizeof(char), size, f);
    if (result != size)
        throw std::runtime_error("Error reading file " + std::string(path));

    delete[] str;
    return str;
}

int main() {
    Timer timer;

    char* json = readFile("data/countries.geojson");
    timer("read file");

    const auto features = mapbox::geojson::parse(json).get<mapbox::geojson::feature_collection>();
    timer("parse into geometry");

    mapbox::geojsonvt::Options options;
    options.indexMaxZoom = 7;
    options.indexMaxPoints = 200;

    mapbox::geojsonvt::GeoJSONVT index{ features, options };
    timer("index");

    printf("tiles generated: %i {\n", static_cast<int>(index.total));
    for (const auto& pair : index.stats) {
        printf("    z%i: %i\n", pair.first, pair.second);
    }
    printf("}\n");
}
