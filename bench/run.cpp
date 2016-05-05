#include <mapbox/geojsonvt.hpp>
#include <memory>
#include <cmath>
#include "util.hpp"

using namespace mapbox::geojsonvt;

int main() {

    unsigned iterations = 1000;

    std::string filename("data/countries.geojson");
    std::string data = loadFile(filename);
    // load once before benchmarking to warm up
    const auto features = GeoJSONVT::convertFeatures(data);
    GeoJSONVT vt(features);

    Timer timer;

    for (unsigned i = 0; i < iterations; ++i) {
        GeoJSONVT::convertFeatures(data);
    }
    timer.report("convertFeatures");

    for (unsigned i = 0; i < iterations; ++i) {
        GeoJSONVT vt_(features);
    }
    timer.report("parse");
    const unsigned max_z = 12; // z 0...12 (5592405 tiles)
    std::size_t count = 0;
    for (unsigned z = 0; z < max_z; ++z)
    {
        unsigned num_tiles = std::pow(2, z);
        for (unsigned x = 0; x < num_tiles; ++x)
        {
            for (unsigned y = 0; y < num_tiles; ++y)
            {
                auto const& tile = vt.getTile(z, x, y);
                if (tile) ++count;
            }
        }
    }
    timer.report("getTile");
}
