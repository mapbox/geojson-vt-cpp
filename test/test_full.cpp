#include "util.hpp"
#include <gtest/gtest.h>

#include <mapbox/geojsonvt.hpp>
#include <mapbox/geojsonvt/convert.hpp>
#include <mapbox/geojsonvt/tile.hpp>

#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <stdexcept>

using namespace mapbox::geojsonvt;

std::map<std::string, std::vector<TileFeature>>
genTiles(const std::string& data, uint8_t maxZoom = 0, uint32_t maxPoints = 10000) {
    Options options;
    options.maxZoom = 14;
    options.indexMaxZoom = maxZoom;
    options.indexMaxPoints = maxPoints;

    const auto features = GeoJSONVT::convertFeatures(data, options);
    GeoJSONVT index{ features, options };

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
    Arguments(const std::string inputFile_,
              const std::string expectedFile_,
              const uint32_t maxZoom_ = 0,
              const uint32_t maxPoints_ = 10000)
        : inputFile(inputFile_),
          expectedFile(expectedFile_),
          maxZoom(maxZoom_),
          maxPoints(maxPoints_){};

    const std::string inputFile;
    const std::string expectedFile;
    const uint32_t maxZoom;
    const uint32_t maxPoints;
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

TEST(GenTiles, InvalidGeoJSON) {
    try {
        genTiles("{\"type\": \"Pologon\"}");
        FAIL() << "Expected exception";
    } catch (const std::runtime_error& ex) {
        ASSERT_STREQ("No coordinates in a GeoJSON geometry.", ex.what());
    }
}

TEST(GenTiles, EmptyGeoJSON) {
    const auto tiles = genTiles(loadFile("test/fixtures/empty.json"));
    ASSERT_EQ(0, tiles.size());
}


TEST(GenTiles, NoObjectGeoJSON) {
    try {
        genTiles("42");
        FAIL() << "Expected exception";
    } catch (const std::runtime_error& ex) {
        ASSERT_STREQ("Root of GeoJSON must be an object.", ex.what());
    }
}

INSTANTIATE_TEST_CASE_P(
    Full,
    TileTest,
    ::testing::ValuesIn(std::vector<Arguments>{
        { "test/fixtures/us-states.json", "test/fixtures/us-states-tiles.json", 7, 200 },
        { "test/fixtures/dateline.json", "test/fixtures/dateline-tiles.json", 7, 200 },
        { "test/fixtures/feature.json", "test/fixtures/feature-tiles.json" },
        { "test/fixtures/collection.json", "test/fixtures/collection-tiles.json" },
        { "test/fixtures/single-geom.json", "test/fixtures/single-geom-tiles.json" } }));
