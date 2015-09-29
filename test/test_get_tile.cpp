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

TEST(GetTile, USStates_z7_37_48) {
    const auto features = GeoJSONVT::convertFeatures(loadFile("test/fixtures/us-states.json"));
    GeoJSONVT index{ features };

    const auto actual = index.getTile(7, 37, 48).features;
    const auto expected = parseJSONTile(loadFile("test/fixtures/us-states-z7-37-48.json"));
    ASSERT_EQ(expected, actual);
}
