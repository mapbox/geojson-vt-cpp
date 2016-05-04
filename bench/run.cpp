#include <mapbox/geojsonvt.hpp>
#include <memory>
#include "util.hpp"

using namespace mapbox::geojsonvt;

int main() {

    unsigned iterations = 1000;

    std::string filename("data/countries.geojson");
    std::string data = loadFile(filename);
    // load once before benchmarking to warm up
    const auto features = GeoJSONVT::convertFeatures(data);
    std::unique_ptr<GeoJSONVT> vt = std::make_unique<GeoJSONVT>(features);

    Timer timer;

    for (unsigned i=0;i<iterations;++i) {
        GeoJSONVT::convertFeatures(data);
    }
    timer.report("convertFeatures");

    for (unsigned i=0;i<iterations;++i) {
        std::make_unique<GeoJSONVT>(features);
    }
    timer.report("parse");

    for (unsigned z=0;z<255;++z) {
        for (unsigned x=0;x<255;++x) {
            for (unsigned y=0;y<255;++y) {
                vt->getTile(z, x, y);
            }
        }
    }
    timer.report("getTile");        
}