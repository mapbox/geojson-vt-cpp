#include "util.hpp"

#include <gtest/gtest.h>

#include <mapbox/variant_io.hpp>

#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

#include <fstream>
#include <sstream>

namespace mapbox {
namespace util {
namespace geojsonvt {

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

::std::ostream& operator<<(::std::ostream& os, ProjectedFeatureType t) {
    switch (t) {
    case ProjectedFeatureType::Point:
        return os << "Point";
    case ProjectedFeatureType::LineString:
        return os << "LineString";
    case ProjectedFeatureType::Polygon:
        return os << "Polygon";
    }
}

::std::ostream& operator<<(::std::ostream& os, const TilePoint& p) {
    return os << "[" << p.x << "," << p.y << "]";
}

::std::ostream& operator<<(::std::ostream& os, const TileRing& r) {
    os << "Ring {";
    for (const auto& pt : r.points) {
        os << pt << ",";
    }
    return os << " }";
}

::std::ostream& operator<<(::std::ostream& os, const TileFeature& f) {
    os << "Feature (" << f.type << ") {";
    for (const auto& g : f.tileGeometry) {
        os << g << ",";
    }
    return os << "}";
}

::std::ostream& operator<<(::std::ostream& os, const ProjectedPoint& p) {
    return os << "[" << p.x << "," << p.y << "," << p.z << "]";
}

::std::ostream& operator<<(::std::ostream& os, const ProjectedFeature& f) {
    return os << "Feature (" << f.type << "): " << f.geometry;
}

::std::ostream& operator<<(::std::ostream& os, const ProjectedPoints& points) {
    os << "ProjectedPoints( points: ";
    for (const auto& p : points) {
        os << p << ",";
    }
    return os << ")";
}

::std::ostream& operator<<(::std::ostream& os, const ProjectedRings& rings) {
    os << "ProjectedRings( rings: ";
    for (const auto& ring : rings) {
        os << ring << ",";
    }
    return os << ")";
}

::std::ostream& operator<<(::std::ostream& os, const ProjectedRing& c) {
    os << "ProjectedPoints( area: " << c.area << ", dist: " << c.dist << ", points: ";
    for (const auto& p : c.points) {
        os << p << ",";
    }
    return os << ")";
}

bool operator==(const TilePoint& a, const TilePoint& b) {
    EXPECT_EQ(a.x, b.x);
    EXPECT_EQ(a.y, b.y);
    return true;
}

bool operator==(const TileRing& a, const TileRing& b) {
    EXPECT_EQ(a.points, b.points);
    return true;
}

bool operator==(const TileFeature& a, const TileFeature& b) {
    EXPECT_EQ(a.type, b.type);
    EXPECT_EQ(a.tileGeometry, b.tileGeometry);
    EXPECT_EQ(a.tags, b.tags);
    return true;
}

bool operator==(const ProjectedFeature& a, const ProjectedFeature& b) {
    EXPECT_EQ(a.type, b.type);
    EXPECT_EQ(a.geometry, b.geometry);
    EXPECT_EQ(a.max, b.max);
    EXPECT_EQ(a.min, b.min);
    EXPECT_EQ(a.tags, b.tags);
    return true;
}

bool operator==(const ProjectedRing& a, const ProjectedRing& b) {
    EXPECT_DOUBLE_EQ(a.area, b.area);
    EXPECT_DOUBLE_EQ(a.dist, b.dist);
    EXPECT_EQ(a.points, b.points);
    return true;
}

std::vector<TileFeature> parseJSONTile(const rapidjson::Value& tile) {
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
                    tileTags.emplace(
                        tagKey, std::string{ jt->value.GetString(), jt->value.GetStringLength() });
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
                if (tileType == TileFeatureType::Point) {
                    const auto& pt = geometry[j];
                    EXPECT_TRUE(pt.IsArray());
                    EXPECT_TRUE(pt.Size() >= 2);
                    EXPECT_TRUE(pt[0].IsNumber());
                    EXPECT_TRUE(pt[1].IsNumber());
                    tileFeature.tileGeometry.emplace_back(
                        TilePoint{ static_cast<int16_t>(pt[0].GetInt()),
                                   static_cast<int16_t>(pt[1].GetInt()) });
                } else {
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
        }

        features.emplace_back(std::move(tileFeature));
    }

    return std::move(features);
}

std::vector<TileFeature> parseJSONTile(const std::string& data) {
    rapidjson::Document d;
    d.Parse<0>(data.c_str());

    if (d.HasParseError()) {
        std::stringstream message;
        message << "JSON error at " << d.GetErrorOffset() << " - "
                << rapidjson::GetParseError_En(d.GetParseError());
        throw std::runtime_error(message.str());
    }
    return parseJSONTile(d);
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

        result.emplace(key, parseJSONTile(tile));
    }

    return result;
}

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
