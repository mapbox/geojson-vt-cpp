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

    auto const& type = data["type"];
    if (type == "FeatureCollection") {
        if (data.HasMember("features")) {
            const JSValue& rawFeatures = data["features"];
            if (rawFeatures.IsArray()) {
                auto size = rawFeatures.Size();
                for (rapidjson::SizeType i = 0; i < size; ++i) {
                    convertFeature(features, rawFeatures[i], tolerance);
                }
            }
        }

    } else if (type == "Feature") {
        convertFeature(features, data, tolerance);

    } else if (type == "GeometryCollection") {
        // geometry collection
        if (data.HasMember("geometries")) {
            const JSValue& rawGeometries = data["geometries"];
            if (rawGeometries.IsArray()) {
                Tags tags;
                auto size = rawGeometries.Size();
                for (rapidjson::SizeType i = 0; i < size; ++i) {
                    convertGeometry(features, tags, rawGeometries[i], tolerance);
                }
            }
        }
    } else {
        // single geometry or a geometry collection
        Tags tags;
        convertGeometry(features, tags, data, tolerance);
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
    auto const& type = geom["type"];

    if (!geom.HasMember("coordinates")) {
        throw std::runtime_error("No coordinates in a GeoJSON geometry.");
    }

    const JSValue& rawCoords = validArray(geom["coordinates"]);

    if (type == "Point") {
        ProjectedPoints points = { projectPoint(readCoordinate(rawCoords)) };
        features.push_back(create(tags, ProjectedFeatureType::Point, points));

    } else if (type == "MultiPoint") {
        ProjectedPoints points;
        auto size = rawCoords.Size();
        for (rapidjson::SizeType i = 0; i < size; ++i) {
            points.push_back(projectPoint(readCoordinate(rawCoords[i])));
        }
        features.push_back(create(tags, ProjectedFeatureType::Point, points));

    } else if (type ==  "LineString") {
        ProjectedRing ring{ readCoordinateRing(rawCoords, tolerance) };
        ProjectedRings rings{ ring };
        features.push_back(create(tags, ProjectedFeatureType::LineString, rings));

    } else if (type == "MultiLineString" || type == "Polygon") {
        ProjectedRings rings;
        auto size = rawCoords.Size();
        for (rapidjson::SizeType i = 0; i < size; ++i) {
            rings.push_back(readCoordinateRing(validArray(rawCoords[i]), tolerance));
        }

        ProjectedFeatureType projectedType =
            ((type == "Polygon") ? ProjectedFeatureType::Polygon : ProjectedFeatureType::LineString);

        features.push_back(create(tags, projectedType, rings));
    }

    else if (type == "MultiPolygon") {
        ProjectedRings rings;
        auto size = rawCoords.Size();
        for (rapidjson::SizeType k = 0; k < size; ++k) {
            const JSValue& rawRings = validArray(rawCoords[k]);
            auto ringsSize = rawRings.Size();
            for (rapidjson::SizeType i = 0; i < ringsSize; ++i) {
                rings.push_back(readCoordinateRing(validArray(rawRings[i]), tolerance));
            }
        }

        features.push_back(create(tags, ProjectedFeatureType::Polygon, rings));

    } else if (type == "GeometryCollection") {
        if (geom.HasMember("geometries")) {
            const JSValue& rawGeometries = validArray(geom["geometries"]);
            auto size = rawGeometries.Size();
            for (rapidjson::SizeType i = 0; i < size; ++i) {
                convertGeometry(features, tags, rawGeometries[i], tolerance);
            }
        }

    } else {
        throw std::runtime_error("Input data is not a valid GeoJSON object");
    }
}

geometry::point<double> Convert::readCoordinate(const JSValue& value) {
    return geometry::point<double>(value[0].GetDouble(), value[1].GetDouble());

}

ProjectedRing Convert::readCoordinateRing(const JSValue& rawRing, double tolerance) {
    geometry::linear_ring<double> ring;
    auto size = rawRing.Size();
    ring.reserve(size);
    for (rapidjson::SizeType j = 0; j < size; ++j) {
        ring.push_back(readCoordinate(rawRing[j]));
    }
    return projectRing(ring, tolerance);
}

const JSValue& Convert::validArray(const JSValue& value, rapidjson::SizeType minSize) {
    if (!value.IsArray() || value.Size() < minSize) {
        throw std::runtime_error("Invalid GeoJSON coordinates");
    }
    return value;
}

ProjectedFeature Convert::create(Tags const& tags, ProjectedFeatureType type, ProjectedGeometry const& geometry) {
    ProjectedFeature feature(geometry, type, tags);
    calcBBox(feature);

    return feature;
}

ProjectedRing Convert::projectRing(geometry::linear_ring<double> const& points, double tolerance) {

    ProjectedRing ring;
    for (auto const& pt : points) {
        ring.points.push_back(projectPoint(pt));
    }
    Simplify::simplify(ring.points, tolerance);
    calcSize(ring);

    return ring;
}

ProjectedPoint Convert::projectPoint(geometry::point<double> const& pt) {

    double sine = std::sin(pt.y * M_PI / 180);
    double x = pt.x / 360 + 0.5;
    double y = 0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI;
    y = y < 0 ? 0 : y > 1 ? 1 : y;
    return ProjectedPoint(x, y, 0);
}

// calculate area and length of the poly
void Convert::calcSize(ProjectedRing& ring) {
    double area = 0, dist = 0;
    ProjectedPoint a, b;
    auto size =  ring.points.size();
    for (size_t i = 0; i < size - 1; ++i) {
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
