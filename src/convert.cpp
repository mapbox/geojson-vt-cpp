#include <mapbox/geojsonvt/convert.hpp>
#include <mapbox/geojsonvt/simplify.hpp>

#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace {

template <typename T>
std::string to_string(const T& value) {
    std::ostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(6);
    stream << value;
    return stream.str();
}

} // namespace

namespace mapbox {
namespace geojsonvt {

// converts GeoJSON feature into an intermediate projected JSON vector format with simplification
// data

std::vector<ProjectedFeature> Convert::convert(const JSValue& data, double tolerance) {
    std::vector<ProjectedFeature> features;

    if (!data.IsObject()) {
        throw std::runtime_error("Root of GeoJSON must be an object.");
    }

    if (!data.HasMember("type")) {
        throw std::runtime_error("No type in a GeoJSON object.");
    }

    const JSValue& rawType = data["type"];

    if (std::string(rawType.GetString()) == "FeatureCollection") {
        if (data.HasMember("features")) {
            const JSValue& rawFeatures = data["features"];
            if (rawFeatures.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawFeatures.Size(); ++i) {
                    convertFeature(features, rawFeatures[i], tolerance);
                }
            }
        }

    } else if (std::string(data["type"].GetString()) == "Feature") {
        convertFeature(features, data, tolerance);

    } else if (std::string(data["type"].GetString()) == "GeometryCollection") {
        // geometry collection
        if (data.HasMember("geometries")) {
            const JSValue& rawGeometries = data["geometries"];
            if (rawGeometries.IsArray()) {
                Tags tags;
                for (rapidjson::SizeType i = 0; i < rawGeometries.Size(); ++i) {
                    convertGeometry(features, tags, rawGeometries[i], tolerance);
                }
            }
        }
    } else {
        // single geometry or a geometry collection
        convertGeometry(features, {}, data, tolerance);
    }

    return features;
}

void Convert::convertFeature(std::vector<ProjectedFeature>& features,
                             const JSValue& feature,
                             double tolerance) {
    Tags tags;
    if (feature.HasMember("properties") && feature["properties"].IsObject()) {
        const JSValue& properties = feature["properties"];
        JSValue::ConstMemberIterator itr = properties.MemberBegin();
        for (; itr != properties.MemberEnd(); ++itr) {
            std::string key{ itr->name.GetString(), itr->name.GetStringLength() };
            switch (itr->value.GetType()) {
            case rapidjson::kNullType:
                tags.emplace(key, "null");
                break;
            case rapidjson::kFalseType:
                tags.emplace(key, "false");
                break;
            case rapidjson::kTrueType:
                tags.emplace(key, "true");
                break;
            case rapidjson::kStringType:
                tags.emplace(key,
                             std::string{ itr->value.GetString(), itr->value.GetStringLength() });
                break;
            case rapidjson::kNumberType:
                tags.emplace(key, to_string(itr->value.GetDouble()));
                break;
            default:
                // it's either array or object, but both of those are invalid
                // in this context
                break;
            }
        }
    }

    if (feature.HasMember("geometry") && feature["geometry"].IsObject()) {
        convertGeometry(features, tags, feature["geometry"], tolerance);
    }
}

void Convert::convertGeometry(std::vector<ProjectedFeature>& features,
                              const Tags& tags,
                              const JSValue& geom,
                              double tolerance) {
    if (!geom.HasMember("type")) {
        throw std::runtime_error("No type in a GeoJSON geometry.");
    }

    const JSValue& rawType = geom["type"];
    std::string type{ rawType.GetString(), rawType.GetStringLength() };

    if (!geom.HasMember("coordinates")) {
        throw std::runtime_error("No coordinates in a GeoJSON geometry.");
    }

    const JSValue& rawCoords = validArray(geom["coordinates"]);

    if (type == "Point") {
        ProjectedPoints points = { projectPoint(LonLat(readCoordinate(rawCoords))) };
        features.push_back(create(tags, ProjectedFeatureType::Point, points));

    } else if (type == "MultiPoint") {
        ProjectedPoints points;

        for (rapidjson::SizeType i = 0; i < rawCoords.Size(); ++i) {
            points.push_back(projectPoint(LonLat(readCoordinate(rawCoords[i]))));
        }
        features.push_back(create(tags, ProjectedFeatureType::Point, points));

    } else if (type == "LineString") {
        ProjectedRing ring{ readCoordinateRing(rawCoords, tolerance) };
        ProjectedRings rings{ ring };
        features.push_back(create(tags, ProjectedFeatureType::LineString, rings));

    } else if (type == "MultiLineString" || type == "Polygon") {
        ProjectedRings rings;

        for (rapidjson::SizeType i = 0; i < rawCoords.Size(); ++i) {
            rings.push_back(readCoordinateRing(validArray(rawCoords[i]), tolerance));
        }

        ProjectedFeatureType projectedType =
            (type == "Polygon" ? ProjectedFeatureType::Polygon : ProjectedFeatureType::LineString);

        features.push_back(create(tags, projectedType, rings));
    }

    else if (type == "MultiPolygon") {
        ProjectedRings rings;

        for (rapidjson::SizeType k = 0; k < rawCoords.Size(); ++k) {
            const JSValue& rawRings = validArray(rawCoords[k]);

            for (rapidjson::SizeType i = 0; i < rawRings.Size(); ++i) {
                rings.push_back(readCoordinateRing(validArray(rawRings[i]), tolerance));
            }
        }

        features.push_back(create(tags, ProjectedFeatureType::Polygon, rings));

    } else if (type == "GeometryCollection") {
        if (geom.HasMember("geometries")) {
            const JSValue& rawGeometries = validArray(geom["geometries"]);

            for (rapidjson::SizeType i = 0; i < rawGeometries.Size(); ++i) {
                convertGeometry(features, tags, rawGeometries[i], tolerance);
            }
        }

    } else {
        throw std::runtime_error("Input data is not a valid GeoJSON object");
    }
}

std::array<double, 2> Convert::readCoordinate(const JSValue& value) {
    validArray(value, 2);
    return { { value[static_cast<rapidjson::SizeType>(0)].GetDouble(),
               value[static_cast<rapidjson::SizeType>(1)].GetDouble() } };
}

ProjectedRing Convert::readCoordinateRing(const JSValue& rawRing, double tolerance) {
    std::vector<LonLat> points;

    for (rapidjson::SizeType j = 0; j < rawRing.Size(); ++j) {
        points.push_back(LonLat(readCoordinate(rawRing[j])));
    }
    return projectRing(points, tolerance);
}

const JSValue& Convert::validArray(const JSValue& value, rapidjson::SizeType minSize) {
    if (!value.IsArray() || value.Size() < minSize) {
        throw std::runtime_error("Invalid GeoJSON coordinates");
    }
    return value;
}

ProjectedFeature Convert::create(Tags tags, ProjectedFeatureType type, ProjectedGeometry geometry) {
    ProjectedFeature feature(geometry, type, tags);
    calcBBox(feature);

    return feature;
}

ProjectedRing Convert::projectRing(const std::vector<LonLat>& lonlats, double tolerance) {

    ProjectedRing ring;
    for (auto lonlat : lonlats) {
        ring.points.push_back(projectPoint(lonlat));
    }

    Simplify::simplify(ring.points, tolerance);
    calcSize(ring);

    return ring;
}

ProjectedPoint Convert::projectPoint(const LonLat& p_) {

    double sine = std::sin(p_.lat * M_PI / 180);
    double x = p_.lon / 360 + 0.5;
    double y = 0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI;

    y = y < 0 ? 0 : y > 1 ? 1 : y;

    return ProjectedPoint(x, y, 0);
}

// calculate area and length of the poly
void Convert::calcSize(ProjectedRing& ring) {
    double area = 0, dist = 0;
    ProjectedPoint a, b;

    for (size_t i = 0; i < ring.points.size() - 1; ++i) {
        a = (b.isValid() ? b : ring.points[i]);
        b = ring.points[i + 1];

        area += a.x * b.y - b.x * a.y;

        // use Manhattan distance instead of Euclidian one to avoid expensive square root
        // computation
        dist += std::abs(b.x - a.x) + std::abs(b.y - a.y);
    }

    ring.area = std::abs(area / 2);
    ring.dist = dist;
}

// calculate the feature bounding box for faster clipping later
void Convert::calcBBox(ProjectedFeature& feature) {
    auto& min = feature.min;
    auto& max = feature.max;

    if (feature.type == ProjectedFeatureType::Point) {
        calcRingBBox(min, max, feature.geometry.get<ProjectedPoints>());

    } else {
        auto& rings = feature.geometry.get<ProjectedRings>();
        for (auto& ring : rings) {
            calcRingBBox(min, max, ring.points);
        }
    }
}

void Convert::calcRingBBox(ProjectedPoint& minPoint,
                           ProjectedPoint& maxPoint,
                           const ProjectedPoints& points) {
    for (auto& p : points) {
        minPoint.x = std::min(p.x, minPoint.x);
        maxPoint.x = std::max(p.x, maxPoint.x);
        minPoint.y = std::min(p.y, minPoint.y);
        maxPoint.y = std::max(p.y, maxPoint.y);
    }
}

} // namespace geojsonvt
} // namespace mapbox
