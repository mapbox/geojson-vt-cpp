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

const auto square = parseJSONTile(loadFile("test/fixtures/us-states-square.json"));

TEST(GetTile, USStates) {
    const auto features = GeoJSONVT::convertFeatures(loadFile("test/fixtures/us-states.json"));
    GeoJSONVT index{ features };

    const auto tile1 = index.getTile(7, 37, 48).features;
    const auto tile2 = index.getTile(9, 148, 192).features;
    const auto tile3 = index.getTile(11, 592, 768).features;

    ASSERT_EQ(parseJSONTile(loadFile("test/fixtures/us-states-z7-37-48.json")), tile1);
    ASSERT_EQ(square, tile2); // clipped square
    ASSERT_EQ(square, tile3); // clipped square
}
