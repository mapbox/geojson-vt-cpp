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

::std::ostream& operator<<(::std::ostream& os, const ProjectedPoint& p) {
    return os << "[" << p.x << "," << p.y << "," << p.z << "]";
}

::std::ostream& operator<<(::std::ostream& os, const ProjectedGeometryContainer& c) {
    os << "{ Container( area: " << c.area << ", dist: " << c.dist << ", members: ";
    for (const auto& member : c.members) {
        os << member << ",";
    }
    return os << " }";
}

bool operator==(const ProjectedFeature& a, const ProjectedFeature& b) {
    EXPECT_EQ(a.type, b.type);
    EXPECT_EQ(a.geometry, b.geometry);
    EXPECT_EQ(a.maxPoint, b.maxPoint);
    EXPECT_EQ(a.minPoint, b.minPoint);
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
