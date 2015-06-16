#include "util.hpp"
#include <mapbox/geojsonvt/geojsonvt_clip.hpp>
#include <gtest/gtest.h>

using namespace mapbox::util::geojsonvt;

ProjectedPoint intersectX(const ProjectedPoint& p0, const ProjectedPoint& p1, double x) {
    return { x, (x - p0.x) * (p1.y - p0.y) / (p1.x - p0.x) + p0.y };
}

using F = ProjectedFeature;
using P = ProjectedPoint;
using C = ProjectedGeometryContainer;
using G = std::vector<ProjectedGeometry>;

const auto geom1 = G{ G{ P{ 0, 0 },
                         P{ 50, 0 },
                         P{ 50, 10 },
                         P{ 20, 10 },
                         P{ 20, 20 },
                         P{ 30, 20 },
                         P{ 30, 30 },
                         P{ 50, 30 },
                         P{ 50, 40 },
                         P{ 25, 40 },
                         P{ 25, 50 },
                         P{ 0, 50 },
                         P{ 0, 60 },
                         P{ 25, 60 } } };
const auto geom2 = G{ G{ P{ 0, 0 }, P{ 50, 0 }, P{ 50, 10 }, P{ 0, 10 } } };

TEST(Clip, Polylines) {
    const std::map<std::string, std::string> tags1;
    const std::map<std::string, std::string> tags2;

    const std::vector<F> features{
        F{ geom1, ProjectedFeatureType::LineString, tags1 },
        F{ geom2, ProjectedFeatureType::LineString, tags2 },
    };

    const auto clipped = Clip::clip(features, 1, 10, 40, 0, intersectX);

    const std::vector<ProjectedFeature> expected = {
        { G{
             G{ P{ 10, 0 }, P{ 40, 0 } },
             G{ P{ 40, 10 }, P{ 20, 10 }, P{ 20, 20 }, P{ 30, 20 }, P{ 30, 30 }, P{ 40, 30 } },
             G{ P{ 40, 40 }, P{ 25, 40 }, P{ 25, 50 }, P{ 10, 50 } },
             G{ P{ 10, 60 }, P{ 25, 60 } },
          },
          ProjectedFeatureType::LineString,
          tags1 },
        { G{
             G{ P{ 10, 0 }, P{ 40, 0 } }, G{ P{ 40, 10 }, P{ 10, 10 } },
          },
          ProjectedFeatureType::LineString,
          tags2 }
    };

    ASSERT_EQ(expected, clipped);
}

std::vector<ProjectedGeometry> closed(std::vector<ProjectedGeometry> geometry) {
    for (auto& geom : geometry) {
        auto& a = geom.get<ProjectedGeometryContainer>().members;
        a.push_back(a.front());
    }
    return geometry;
}

TEST(Clip, Polygon) {
    const std::map<std::string, std::string> tags1;
    const std::map<std::string, std::string> tags2;

    const std::vector<F> features{
        F{ closed(geom1), ProjectedFeatureType::Polygon, tags1 },
        F{ closed(geom2), ProjectedFeatureType::Polygon, tags2 },
    };

    const auto clipped = Clip::clip(features, 1, 10, 40, 0, intersectX);

    const std::vector<ProjectedFeature> expected = {
        { G{ G{ P{ 10, 0 },
                P{ 40, 0 },
                P{ 40, 10 },
                P{ 20, 10 },
                P{ 20, 20 },
                P{ 30, 20 },
                P{ 30, 30 },
                P{ 40, 30 },
                P{ 40, 40 },
                P{ 25, 40 },
                P{ 25, 50 },
                P{ 10, 50 },
                P{ 10, 60 },
                P{ 25, 60 },
                P{ 10, 24 },
                P{ 10, 0 } }

          },
          ProjectedFeatureType::Polygon,
          tags1 },
        { G{ G{ P{ 10, 0 }, P{ 40, 0 }, P{ 40, 10 }, P{ 10, 10 }, P{ 10, 0 } } },
          ProjectedFeatureType::Polygon,
          tags2 }
    };

    ASSERT_EQ(expected, clipped);
}

TEST(Clip, Points) {
    const std::map<std::string, std::string> tags1;
    const std::map<std::string, std::string> tags2;

    const std::vector<F> features{
        F{ geom1[0], ProjectedFeatureType::Point, tags1 },
        F{ geom2[0], ProjectedFeatureType::Point, tags2 },
    };

    const auto clipped = Clip::clip(features, 1, 10, 40, 0, intersectX);

    const std::vector<ProjectedFeature> expected = {
        { G{ P{ 20, 10 },
             P{ 20, 20 },
             P{ 30, 20 },
             P{ 30, 30 },
             P{ 25, 40 },
             P{ 25, 50 },
             P{ 25, 60 } },
          ProjectedFeatureType::Point,
          tags1 },
    };

    ASSERT_EQ(expected, clipped);
}
