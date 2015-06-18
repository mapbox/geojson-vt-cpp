#include "util.hpp"

#include <gtest/gtest.h>

#include <mapbox/variant_io.hpp>

namespace mapbox {
namespace util {
namespace geojsonvt {

::std::ostream& operator<<(::std::ostream& os, ProjectedFeatureType t) {
    switch (t) {
        case ProjectedFeatureType::Point: return os << "Point";
        case ProjectedFeatureType::LineString: return os << "LineString";
        case ProjectedFeatureType::Polygon: return os << "Polygon";
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

::std::ostream& operator<<(::std::ostream& os, const ProjectedGeometryContainer& c) {
    os << "{ Container( area: " << c.area << ", dist: " << c.dist << ", members: ";
    for (const auto& member : c.members) {
        os << member << ",";
    }
    return os << " }";
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

bool operator==(const ProjectedGeometryContainer& a, const ProjectedGeometryContainer& b) {
    EXPECT_DOUBLE_EQ(a.area, b.area);
    EXPECT_DOUBLE_EQ(a.dist, b.dist);
    EXPECT_EQ(a.members, b.members);
    return true;
}

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
