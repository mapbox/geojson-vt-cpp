#include <mapbox/geojsonvt/geojsonvt_convert.hpp>
#include <mapbox/geojsonvt/geojsonvt_simplify.hpp>

#include <array>
#include <cmath>
#include <vector>

namespace mapbox {
namespace util {
namespace geojsonvt {

// converts GeoJSON feature into an intermediate projected JSON vector format with simplification
// data

std::vector<ProjectedFeature> Convert::convert(const JSDocument& data, double tolerance) {
    std::vector<ProjectedFeature> features;
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
        throw std::runtime_error("Input data is not a valid GeoJSON object");
    }

    return std::move(features);
}

void Convert::convertFeature(std::vector<ProjectedFeature>& features,
                             const JSValue& feature,
                             double tolerance) {
    Tags tags;
    if (feature.HasMember("properties") && feature["properties"].IsObject()) {
        const JSValue& properties = feature["properties"];
        rapidjson::Value::ConstMemberIterator itr = properties.MemberBegin();
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
                tags.emplace(key, std::to_string(itr->value.GetDouble()));
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
    const JSValue& rawType = geom["type"];
    std::string type{ rawType.GetString(), rawType.GetStringLength() };

    if (type == "Point") {
        std::array<double, 2> coordinates = { { 0, 0 } };
        if (geom.HasMember("coordinates")) {
            const JSValue& rawCoordinates = geom["coordinates"];
            if (rawCoordinates.IsArray()) {
                coordinates[0] = rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                coordinates[1] = rawCoordinates[(rapidjson::SizeType)1].GetDouble();
            }
        }
        ProjectedPoints points = { projectPoint(LonLat(coordinates)) };

        features.push_back(create(tags, ProjectedFeatureType::Point, points));

    } else if (type == "MultiPoint") {
        std::vector<std::array<double, 2>> coordinatePairs;
        ProjectedPoints points;
        if (geom.HasMember("coordinates")) {
            const JSValue& rawCoordinatePairs = geom["coordinates"];
            if (rawCoordinatePairs.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawCoordinatePairs.Size(); ++i) {
                    std::array<double, 2> coordinates = { { 0, 0 } };
                    const JSValue& rawCoordinates = rawCoordinatePairs[i];
                    if (rawCoordinates.IsArray()) {
                        coordinates[0] = rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                        coordinates[1] = rawCoordinates[(rapidjson::SizeType)1].GetDouble();
                    }
                    points.push_back(projectPoint(LonLat(coordinates)));
                }
            }
        }

        features.push_back(create(tags, ProjectedFeatureType::Point, points));

    } else if (type == "LineString") {
        std::vector<std::array<double, 2>> coordinatePairs;
        std::vector<LonLat> points;
        if (geom.HasMember("coordinates")) {
            const JSValue& rawCoordinatePairs = geom["coordinates"];
            if (rawCoordinatePairs.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawCoordinatePairs.Size(); ++i) {
                    std::array<double, 2> coordinates = { { 0, 0 } };
                    const JSValue& rawCoordinates = rawCoordinatePairs[i];
                    if (rawCoordinates.IsArray()) {
                        coordinates[0] = rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                        coordinates[1] = rawCoordinates[(rapidjson::SizeType)1].GetDouble();
                    }
                    points.push_back(LonLat(coordinates));
                }
            }
        }

        ProjectedRing ring{ projectRing(points, tolerance) };
        ProjectedRings rings{ ring };

        features.push_back(create(tags, ProjectedFeatureType::LineString, rings));

    } else if (type == "MultiLineString" || type == "Polygon") {
        ProjectedRings rings;
        if (geom.HasMember("coordinates")) {
            const JSValue& rawLines = geom["coordinates"];
            if (rawLines.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawLines.Size(); ++i) {
                    const JSValue& rawCoordinatePairs = rawLines[i];
                    if (rawCoordinatePairs.IsArray()) {
                        std::vector<LonLat> points;
                        for (rapidjson::SizeType j = 0; j < rawCoordinatePairs.Size(); ++j) {
                            std::array<double, 2> coordinates = { { 0, 0 } };
                            const JSValue& rawCoordinates = rawCoordinatePairs[j];
                            if (rawCoordinates.IsArray()) {
                                coordinates[0] = rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                                coordinates[1] = rawCoordinates[(rapidjson::SizeType)1].GetDouble();
                            }
                            points.push_back(LonLat(coordinates));
                        }
                        rings.push_back(projectRing(points, tolerance));
                    }
                }
            }
        }

        ProjectedFeatureType projectedType =
            (type == "Polygon" ? ProjectedFeatureType::Polygon : ProjectedFeatureType::LineString);

        features.push_back(create(tags, projectedType, rings));
    }

    else if (type == "MultiPolygon") {
        ProjectedRings rings;
        if (geom.HasMember("coordinates")) {
            const JSValue& rawPolygons = geom["coordinates"];
            if (rawPolygons.IsArray()) {
                for (rapidjson::SizeType k = 0; k < rawPolygons.Size(); ++k) {
                    const JSValue& rawLines = rawPolygons[k];
                    if (rawLines.IsArray()) {
                        for (rapidjson::SizeType i = 0; i < rawLines.Size(); ++i) {
                            const JSValue& rawCoordinatePairs = rawLines[i];
                            if (rawCoordinatePairs.IsArray()) {
                                std::vector<LonLat> points;
                                for (rapidjson::SizeType j = 0; j < rawCoordinatePairs.Size();
                                     ++j) {
                                    std::array<double, 2> coordinates = { { 0, 0 } };
                                    const JSValue& rawCoordinates = rawCoordinatePairs[j];
                                    if (rawCoordinates.IsArray()) {
                                        coordinates[0] =
                                            rawCoordinates[(rapidjson::SizeType)0].GetDouble();
                                        coordinates[1] =
                                            rawCoordinates[(rapidjson::SizeType)1].GetDouble();
                                    }
                                    points.push_back(LonLat(coordinates));
                                }
                                rings.push_back(projectRing(points, tolerance));
                            }
                        }
                    }
                }
            }
        }

        features.push_back(create(tags, ProjectedFeatureType::Polygon, rings));

    } else if (type == "GeometryCollection") {
        if (geom.HasMember("geometries")) {
            const JSValue& rawGeometries = geom["geometries"];
            if (rawGeometries.IsArray()) {
                for (rapidjson::SizeType i = 0; i < rawGeometries.Size(); ++i) {
                    convertGeometry(features, tags, rawGeometries[i], tolerance);
                }
            }
        }

    } else {
        throw std::runtime_error("Input data is not a valid GeoJSON object");
    }
}

ProjectedFeature Convert::create(Tags tags, ProjectedFeatureType type, ProjectedGeometry geometry) {
    ProjectedFeature feature(geometry, type, tags);
    calcBBox(feature);

    return std::move(feature);
}

ProjectedRing Convert::projectRing(const std::vector<LonLat>& lonlats, double tolerance) {

    ProjectedRing ring;
    for (size_t i = 0; i < lonlats.size(); ++i) {
        ring.points.push_back(projectPoint(lonlats[i]));
    }

    Simplify::simplify(ring.points, tolerance);
    calcSize(ring);

    return std::move(ring);
}

ProjectedPoint Convert::projectPoint(const LonLat& p_) {

    double sine = std::sin(p_.lat * M_PI / 180);
    double x = p_.lon / 360 + 0.5;
    double y = 0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI;

    y = y < -1 ? -1 : y > 1 ? 1 : y;

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
} // namespace util
} // namespace mapbox
