#include <mapbox/geojson.hpp>
#include <mapbox/geojsonvt.hpp>

#include <iostream>
#include <fstream>

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

std::string readFile(std::string const& path) {
    std::ifstream stream(path.c_str(),std::ios_base::in|std::ios_base::binary);
    if (!stream.is_open())
    {
        throw std::runtime_error("could not open: '" + path + "'");
    }
    std::string buffer(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));
    stream.close();
    return buffer;
}

int main() {
    Timer timer;

    const std::string json = readFile("data/countries.geojson");
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

    const unsigned max_z = 10;
    std::size_t count = 0;
    for (unsigned z = 0; z < max_z; ++z) {
        unsigned num_tiles = std::pow(2, z);
        for (unsigned x = 0; x < num_tiles; ++x) {
            for (unsigned y = 0; y < num_tiles; ++y) {
                auto const& tile = index.getTile(z, x, y);
                if (!tile.features.empty())
                    ++count;
            }
        }
    }
    timer("found " + std::to_string(count) + " tiles");
}
