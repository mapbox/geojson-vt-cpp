#include "util.hpp"
#include <gtest/gtest.h>

#include <mapbox/geojsonvt/geojsonvt.hpp>
#include <mapbox/geojsonvt/geojsonvt_convert.hpp>
#include <mapbox/geojsonvt/geojsonvt_tile.hpp>

#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <stdexcept>

using namespace mapbox::util::geojsonvt;

std::map<std::string, std::vector<TileFeature>> genTiles(const std::string& data, uint8_t maxZoom = 0, uint32_t maxPoints = 10000) {
    const auto features = GeoJSONVT::convertFeatures(data);
    GeoJSONVT index { features, 14, maxZoom, maxPoints };

    std::map<std::string, std::vector<TileFeature>> output;

    for (const auto& pair : index.getAllTiles()) {
        auto& tile = pair.second;
        const int8_t z = std::log(tile.z2) / M_LN2;
        const std::string key = std::string("z") + std::to_string(z) + "-" +
                                std::to_string(tile.tx) + "-" + std::to_string(tile.ty);

        output.emplace(key, index.getTile(z, tile.tx, tile.ty).features);
    }

    return output;
}

struct Arguments {
    const std::string inputFile;
    const std::string expectedFile;
    const uint32_t maxZoom = 0;
    const uint32_t maxPoints = 10000;
};


::std::ostream& operator<<(::std::ostream& os, const Arguments& a) {
    return os << a.inputFile << " (" << a.maxZoom << ", " << a.maxPoints << ")";
}


class TileTest : public ::testing::TestWithParam<Arguments> {};

TEST_P(TileTest, Tiles) {
    const auto& params = GetParam();

    const auto actual = genTiles(loadFile(params.inputFile), params.maxZoom, params.maxPoints);
    const auto expected = parseJSONTiles(loadFile(params.expectedFile));

    ASSERT_EQ(expected, actual);
}

INSTANTIATE_TEST_CASE_P(Full, TileTest, ::testing::ValuesIn(std::vector<Arguments>{
    { "test/fixtures/us-states.json", "test/fixtures/us-states-tiles.json", 7, 200 },
    { "test/fixtures/dateline.json", "test/fixtures/dateline-tiles.json", 7, 200 },
    { "test/fixtures/feature.json", "test/fixtures/feature-tiles.json" },
    { "test/fixtures/collection.json", "test/fixtures/collection-tiles.json" },
}));
