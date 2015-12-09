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

const auto square = parseJSONTile(loadFile("test/fixtures/us-states-square.json"));

TEST(GetTile, USStates) {
    const auto features = GeoJSONVT::convertFeatures(loadFile("test/fixtures/us-states.json"));
    GeoJSONVT index{ features };

    ASSERT_EQ(parseJSONTile(loadFile("test/fixtures/us-states-z7-37-48.json")),
              index.getTile(7, 37, 48).features);

    ASSERT_EQ(square, index.getTile(9, 148, 192).features);  // clipped square
    ASSERT_EQ(square, index.getTile(11, 592, 768).features); // clipped square

    ASSERT_EQ(GeoJSONVT::emptyTile, index.getTile(11, 800, 400));   // non-existing tile
    ASSERT_EQ(&GeoJSONVT::emptyTile, &index.getTile(11, 800, 400)); // non-existing tile

    // This test does not make sense in C++, since the parameters are cast to integers anyway.
    // ASSERT_EQ(GeoJSONVT::emptyTile, index.getTile(-5, 123.25, 400.25)); // invalid tile

    ASSERT_EQ(29, index.getTotal());
}
