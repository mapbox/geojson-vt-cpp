#include "util.hpp"
#include <gtest/gtest.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>
#include <mapbox/geojsonvt/geojsonvt.hpp>
#include <mapbox/geojsonvt/geojsonvt_convert.hpp>
#include <mapbox/geojsonvt/geojsonvt_tile.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <stdexcept>


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


using namespace mapbox::util::geojsonvt;

std::map<std::string, std::vector<TileFeature>> genTiles(const std::string& data, uint8_t maxZoom, uint32_t maxPoints) {
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

std::map<std::string, std::vector<TileFeature>> parseJSONTiles(const std::string& data) {
    std::map<std::string, std::vector<TileFeature>> result;

    rapidjson::Document d;
    d.Parse<0>(data.c_str());

    if (d.HasParseError()) {
        std::stringstream message;
        message << "JSON error at " << d.GetErrorOffset() << " - "
                << rapidjson::GetParseError_En(d.GetParseError());
        throw std::runtime_error(message.str());
    }

    EXPECT_TRUE(d.IsObject());
    for (auto it = d.MemberBegin(); it != d.MemberEnd(); it++) {
        const std::string key{ it->name.GetString(), it->name.GetStringLength() };
        const auto& tile = it->value;
        std::vector<TileFeature> features;
        EXPECT_TRUE(tile.IsArray());
        for (rapidjson::SizeType k = 0; k < tile.Size(); ++k) {
            const auto& feature = tile[k];

            TileFeatureType tileType = TileFeatureType::Point;
            if (feature.HasMember("type")) {
                const auto& type = feature["type"];
                EXPECT_TRUE(type.IsInt());
                tileType = TileFeatureType(type.GetInt());
            }

            std::map<std::string, std::string> tileTags;
            if (feature.HasMember("tags")) {
                const auto& tags = feature["tags"];
                EXPECT_TRUE(tags.IsObject());
                for (auto jt = tags.MemberBegin(); jt != tags.MemberEnd(); jt++) {
                    const std::string tagKey{ jt->name.GetString(), jt->name.GetStringLength() };
                    switch (jt->value.GetType()) {
                    case rapidjson::kNullType:
                        tileTags.emplace(tagKey, "null");
                        break;
                    case rapidjson::kFalseType:
                        tileTags.emplace(tagKey, "false");
                        break;
                    case rapidjson::kTrueType:
                        tileTags.emplace(tagKey, "true");
                        break;
                    case rapidjson::kStringType:
                        tileTags.emplace(tagKey, std::string{ jt->value.GetString(),
                                                              jt->value.GetStringLength() });
                        break;
                    case rapidjson::kNumberType:
                        tileTags.emplace(tagKey, std::to_string(jt->value.GetDouble()));
                        break;
                    default:
                        EXPECT_TRUE(false) << "invalid JSON type";
                    }
                }
            }

            TileFeature tileFeature{ {}, tileType, tileTags };
            EXPECT_TRUE(feature.IsObject());
            if (feature.HasMember("geometry")) {
                const auto& geometry = feature["geometry"];
                EXPECT_TRUE(geometry.IsArray());
                for (rapidjson::SizeType j = 0; j < geometry.Size(); ++j) {
                    const auto& ring = geometry[j];
                    EXPECT_TRUE(ring.IsArray());
                    TileRing tileRing;
                    for (rapidjson::SizeType i = 0; i < ring.Size(); ++i) {
                        const auto& pt = ring[i];
                        EXPECT_TRUE(pt.IsArray());
                        EXPECT_TRUE(pt.Size() >= 2);
                        EXPECT_TRUE(pt[0].IsNumber());
                        EXPECT_TRUE(pt[1].IsNumber());
                        tileRing.points.emplace_back(pt[0].GetInt(), pt[1].GetInt());
                    }
                    tileFeature.tileGeometry.push_back(tileRing);
                }
            }

            features.emplace_back(std::move(tileFeature));
        }

        result.emplace(key, features);
    }

    return result;
}

TEST(Full, Tiles) {
    const std::string inputFile = "test/fixtures/us-states.json";
    const std::string expectedFile = "test/fixtures/us-states-tiles.json";
    const uint32_t maxZoom = 7;
    const uint32_t maxPoints = 200;

    const auto actual = genTiles(loadFile(inputFile), maxZoom, maxPoints);
    const auto expected = parseJSONTiles(loadFile(expectedFile));

    ASSERT_EQ(expected, actual);
}
