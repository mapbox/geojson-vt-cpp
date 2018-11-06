#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include <gtest/gtest.h>
#include <mapbox/geojsonvt/tile.hpp>
#include <mapbox/geojsonvt/types.hpp>
#include <mapbox/geometry.hpp>
#include <mapbox/geometry_io.hpp>
#include <mapbox/variant_io.hpp>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#pragma GCC diagnostic pop

#include <fstream>
#include <map>
#include <sstream>

namespace mapbox {
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

namespace {

struct ToDouble {
    double operator()(double value) const { return value; }
    double operator()(int64_t value) const { return double(value); }
    double operator()(uint64_t value) const { return double(value); }
    template <typename T>
    double operator()(const T&) const { return std::numeric_limits<double>::quiet_NaN(); }
};

template <typename T>
inline void compareValues(const T& a, const T& b) {
    EXPECT_TRUE(a == b);
}

void compareValues(const mapbox::feature::value& a,
                   const mapbox::feature::value& b) {
    if (a == b) return;

    double a_as_double = mapbox::feature::value::visit(a, ToDouble{});
    double b_as_double = mapbox::feature::value::visit(b, ToDouble{});
    EXPECT_DOUBLE_EQ(a_as_double, b_as_double);
}

template <typename MapType>
void compareMaps(const MapType& a, const MapType& b) {
    EXPECT_EQ(a.size(), b.size());
    for (auto it = a.begin(); it != a.end(); ++it) {
        auto found = b.find(it->first);
        if (found != b.end()) {
            compareValues(it->second, found->second);
        } else {
            ADD_FAILURE();
        }
    }
}

} // namespace

bool operator==(const mapbox::feature::feature<short>& a,
                const mapbox::feature::feature<short>& b) {
    // EXPECT_EQ(a.geometry, b.geometry);
    EXPECT_EQ(typeid(a.geometry), typeid(b.geometry));
    compareMaps(a.properties, b.properties);

    EXPECT_EQ(a.id, b.id);
    return true;
}

bool operator==(const mapbox::feature::feature_collection<short>& a,
                const mapbox::feature::feature_collection<short>& b) {
    EXPECT_EQ(a.size(), b.size());
    if (a.size() == b.size()) {
        unsigned i = 0;
        for (i = 0; i < a.size(); i++) {
            EXPECT_EQ(a[i] == b[i], true);
        }
    }
    return true;
}

bool operator==(const std::map<std::string, mapbox::feature::feature_collection<short>>& a,
                const std::map<std::string, mapbox::feature::feature_collection<short>>& b) {
    EXPECT_EQ(a.size(), b.size());
    typedef std::map<std::string, mapbox::feature::feature_collection<short>>::const_iterator
        it_type;
    for (it_type it = a.begin(); it != a.end(); it++) {
        if (b.find(it->first) != b.end()) {
            EXPECT_TRUE(it->second == b.find(it->first)->second);
        } else {
            ADD_FAILURE();
        }
    }
    for (it_type it = b.begin(); it != b.end(); it++) {
        if (a.find(it->first) != a.end()) {
            EXPECT_TRUE(it->second == a.find(it->first)->second);
        } else {
            ADD_FAILURE();
        }
    }
    return true;
}

bool operator==(const mapbox::geojsonvt::Tile& a, const mapbox::geojsonvt::Tile& b) {
    EXPECT_EQ(a.features == b.features, true);
    EXPECT_EQ(a.num_points, b.num_points);
    EXPECT_EQ(a.num_simplified, b.num_simplified);
    return true;
}

mapbox::feature::feature_collection<int16_t>
parseJSONTile(const rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::CrtAllocator>& tile) {
    mapbox::feature::feature_collection<int16_t> features;
    EXPECT_TRUE(tile.IsArray());
    for (rapidjson::SizeType k = 0; k < tile.Size(); ++k) {
        const auto& feature = tile[k];

        mapbox::feature::feature<short> feat{ mapbox::geometry::point<short>() };

        if (feature.HasMember("tags") && feature["tags"].IsObject()) {
            const auto& tags = feature["tags"];
            for (auto jt = tags.MemberBegin(); jt != tags.MemberEnd(); jt++) {
                const std::string tagKey{ jt->name.GetString(), jt->name.GetStringLength() };
                switch (jt->value.GetType()) {
                case rapidjson::kNullType:
                    feat.properties.emplace(tagKey, mapbox::feature::null_value);
                    break;
                case rapidjson::kFalseType:
                    feat.properties.emplace(tagKey, false);
                    break;
                case rapidjson::kTrueType:
                    feat.properties.emplace(tagKey, true);
                    break;
                case rapidjson::kStringType:
                    feat.properties.emplace(
                        tagKey, std::string{ jt->value.GetString(), jt->value.GetStringLength() });
                    break;
                case rapidjson::kNumberType:
                    if (jt->value.IsUint64()) {
                        feat.properties.emplace(tagKey, std::uint64_t(jt->value.GetUint64()));
                    } else if (jt->value.IsInt64()) {
                        feat.properties.emplace(tagKey, std::int64_t(jt->value.GetInt64()));
                    } else {
                        feat.properties.emplace(tagKey, jt->value.GetDouble());
                    }
                    break;
                default:
                    EXPECT_TRUE(false) << "invalid JSON type";
                }
            }
        }

        if (feature.HasMember("id")) {
            EXPECT_TRUE(feature["id"].IsString() || feature["id"].IsNumber());
            if (feature["id"].IsString()) {
                feat.id = { std::string{ feature["id"].GetString(), feature["id"].GetStringLength() } };
            } else {
                feat.id = { std::uint64_t{ feature["id"].GetUint64() } };
            }
        }

        if (feature.HasMember("type") && feature.HasMember("geometry")) {
            const auto& geometry = feature["geometry"];
            const auto& type = feature["type"];
            EXPECT_TRUE(type.IsInt());
            EXPECT_TRUE(geometry.IsArray());
            uint16_t geomType = type.GetInt();
            // point geometry
            if (geomType == 1) {
                for (rapidjson::SizeType j = 0; j < geometry.Size(); ++j) {
                    const auto& pt = geometry[j];
                    EXPECT_TRUE(pt.IsArray());
                    EXPECT_TRUE(pt.Size() >= 2);
                    EXPECT_TRUE(pt[0].IsNumber());
                    EXPECT_TRUE(pt[1].IsNumber());
                    feat.geometry = mapbox::geometry::point<int16_t>(
                        static_cast<int16_t>(pt[0].GetInt()), static_cast<int16_t>(pt[1].GetInt()));
                }
            // linestring geometry
            } else if (geomType == 2) {
                mapbox::geometry::multi_line_string<int16_t> multi_line;
                const bool is_multi = geometry.Size() > 1;
                for (rapidjson::SizeType j = 0; j < geometry.Size(); ++j) {
                    const auto& part = geometry[j];
                    EXPECT_TRUE(part.IsArray());
                    mapbox::geometry::line_string<int16_t> line_string;
                    for (rapidjson::SizeType i = 0; i < part.Size(); ++i) {
                        const auto& pt = part[i];
                        EXPECT_TRUE(pt.IsArray());
                        EXPECT_TRUE(pt.Size() >= 2);
                        EXPECT_TRUE(pt[0].IsNumber());
                        EXPECT_TRUE(pt[1].IsNumber());
                        line_string.emplace_back(static_cast<int16_t>(pt[0].GetInt()),
                                                 static_cast<int16_t>(pt[1].GetInt()));
                    }
                    if (!is_multi) {
                        feat.geometry = line_string;
                        break;
                    } else {
                        multi_line.emplace_back(line_string);
                    }
                }
                if (is_multi)
                    feat.geometry = multi_line;
            // polygon geometry
            } else if (geomType == 3) {
                mapbox::geometry::polygon<int16_t> poly;
                for (rapidjson::SizeType j = 0; j < geometry.Size(); ++j) {
                    const auto& ring = geometry[j];
                    EXPECT_TRUE(ring.IsArray());
                    mapbox::geometry::linear_ring<int16_t> linear_ring;
                    for (rapidjson::SizeType i = 0; i < ring.Size(); ++i) {
                        const auto& pt = ring[i];
                        EXPECT_TRUE(pt.IsArray());
                        EXPECT_TRUE(pt.Size() >= 2);
                        EXPECT_TRUE(pt[0].IsNumber());
                        EXPECT_TRUE(pt[1].IsNumber());
                        linear_ring.emplace_back(static_cast<int16_t>(pt[0].GetInt()),
                                                 static_cast<int16_t>(pt[1].GetInt()));
                    }
                    poly.emplace_back(linear_ring);
                }
                feat.geometry = poly;
            }
        }

        features.emplace_back(feat);
    }

    return features;
}

mapbox::feature::feature_collection<int16_t> parseJSONTile(const std::string& data) {
    rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::CrtAllocator> d;
    d.Parse<0>(data.c_str());

    if (d.HasParseError()) {
        std::stringstream message;
        message << "JSON error at " << d.GetErrorOffset() << " - "
                << rapidjson::GetParseError_En(d.GetParseError());
        throw std::runtime_error(message.str());
    }
    return parseJSONTile(d);
}

std::map<std::string, mapbox::feature::feature_collection<int16_t>>
parseJSONTiles(const std::string& data) {
    std::map<std::string, mapbox::feature::feature_collection<int16_t>> result;
    rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::CrtAllocator> d;
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
} // namespace mapbox
