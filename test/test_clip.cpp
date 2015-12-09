#include "util.hpp"
#include <mapbox/geojsonvt/clip.hpp>
#include <gtest/gtest.h>

using namespace mapbox::geojsonvt;

ProjectedPoint intersectX(const ProjectedPoint& p0, const ProjectedPoint& p1, double x) {
    return { x, (x - p0.x) * (p1.y - p0.y) / (p1.x - p0.x) + p0.y };
}

using F = ProjectedFeature;
using P = ProjectedPoint;
using R = ProjectedRing;
using GR = ProjectedRings;
using GP = ProjectedPoints;

const auto geom1 = GR{ R{ { P{ 0, 0 }, P{ 50, 0 }, P{ 50, 10 }, P{ 20, 10 }, P{ 20, 20 },
                            P{ 30, 20 }, P{ 30, 30 }, P{ 50, 30 }, P{ 50, 40 }, P{ 25, 40 },
                            P{ 25, 50 }, P{ 0, 50 }, P{ 0, 60 }, P{ 25, 60 } } } };

const auto geom2 = GR{ R{ { P{ 0, 0 }, P{ 50, 0 }, P{ 50, 10 }, P{ 0, 10 } } } };

ProjectedPoint min1{ 0, 0 };
ProjectedPoint max1{ 50, 60 };
ProjectedPoint min2{ 0, 0 };
ProjectedPoint max2{ 50, 10 };

TEST(Clip, Polylines) {
    const std::map<std::string, std::string> tags1;
    const std::map<std::string, std::string> tags2;

    const std::vector<F> features{
        F{ geom1, ProjectedFeatureType::LineString, tags1, min1, max1 },
        F{ geom2, ProjectedFeatureType::LineString, tags2, min2, max2 },
    };

    const auto clipped =
        Clip::clip(features, 1, 10, 40, 0, intersectX, -std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity());

    const std::vector<ProjectedFeature> expected = {
        { GR{
              R{ { P{ 10, 0 }, P{ 40, 0 } } },
              R{ { P{ 40, 10 }, P{ 20, 10 }, P{ 20, 20 }, P{ 30, 20 }, P{ 30, 30 }, P{ 40, 30 } } },
              R{ { P{ 40, 40 }, P{ 25, 40 }, P{ 25, 50 }, P{ 10, 50 } } },
              R{ { P{ 10, 60 }, P{ 25, 60 } } },
          },
          ProjectedFeatureType::LineString, tags1, min1, max1 },
        { GR{
              R{ { P{ 10, 0 }, P{ 40, 0 } } }, R{ { P{ 40, 10 }, P{ 10, 10 } } },
          },
          ProjectedFeatureType::LineString, tags2, min2, max2 }
    };

    ASSERT_EQ(expected, clipped);
}

ProjectedGeometry closed(ProjectedGeometry geometry) {
    for (auto& ring : geometry.get<ProjectedRings>()) {
        ring.points.push_back(ring.points.front());
    }
    return geometry;
}

TEST(Clip, Polygon) {
    const std::map<std::string, std::string> tags1;
    const std::map<std::string, std::string> tags2;

    const std::vector<F> features{
        F{ closed(geom1), ProjectedFeatureType::Polygon, tags1, min1, max1 },
        F{ closed(geom2), ProjectedFeatureType::Polygon, tags2, min2, max2 },
    };

    const auto clipped =
        Clip::clip(features, 1, 10, 40, 0, intersectX, -std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity());

    const std::vector<ProjectedFeature> expected = {
        { GR{ R{ { P{ 10, 0 }, P{ 40, 0 }, P{ 40, 10 }, P{ 20, 10 }, P{ 20, 20 }, P{ 30, 20 },
                   P{ 30, 30 }, P{ 40, 30 }, P{ 40, 40 }, P{ 25, 40 }, P{ 25, 50 }, P{ 10, 50 },
                   P{ 10, 60 }, P{ 25, 60 }, P{ 10, 24 }, P{ 10, 0 } } }

          },
          ProjectedFeatureType::Polygon, tags1, min1, max1 },
        { GR{ R{ { P{ 10, 0 }, P{ 40, 0 }, P{ 40, 10 }, P{ 10, 10 }, P{ 10, 0 } } } },
          ProjectedFeatureType::Polygon, tags2, min2, max2 }
    };

    ASSERT_EQ(expected, clipped);
}

TEST(Clip, Points) {
    const std::map<std::string, std::string> tags1;
    const std::map<std::string, std::string> tags2;

    const std::vector<F> features{
        F{ geom1[0].points, ProjectedFeatureType::Point, tags1, min1, max1 },
        F{ geom2[0].points, ProjectedFeatureType::Point, tags2, min2, max2 },
    };

    const auto clipped =
        Clip::clip(features, 1, 10, 40, 0, intersectX, -std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity());

    const std::vector<ProjectedFeature> expected = {
        { GP{ P{ 20, 10 }, P{ 20, 20 }, P{ 30, 20 }, P{ 30, 30 }, P{ 25, 40 }, P{ 25, 50 },
              P{ 25, 60 } },
          ProjectedFeatureType::Point, tags1, min1, max1 },
    };

    ASSERT_EQ(expected, clipped);
}
