#include <mapbox/geojsonvt/tile.hpp>

namespace mapbox {
namespace geojsonvt {

Tile Tile::createTile(std::vector<ProjectedFeature> const& features,
                      uint32_t z2,
                      uint32_t tx,
                      uint32_t ty,
                      double tolerance,
                      bool noSimplify) {
    Tile tile;

    tile.z2 = z2;
    tile.tx = tx;
    tile.ty = ty;

    for (auto const& feature : features) {
        ++tile.numFeatures;
        addFeature(tile, feature, tolerance, noSimplify);

        auto const& min = feature.min;
        auto const& max = feature.max;

        if (min.x < tile.min.x) {
            tile.min.x = min.x;
        }
        if (min.y < tile.min.y) {
            tile.min.y = min.y;
        }
        if (max.x > tile.max.x) {
            tile.max.x = max.x;
        }
        if (max.y > tile.max.y) {
            tile.max.y = max.y;
        }
    }

    return tile;
}

void Tile::addFeature(Tile& tile,
                      ProjectedFeature const& feature,
                      double tolerance,
                      bool noSimplify) {

    const ProjectedFeatureType type = feature.type;
    const double sqTolerance = tolerance * tolerance;

    if (type == ProjectedFeatureType::Point) {
        if (feature.geometry.is<ProjectedPoints>()) {
            auto const& points = feature.geometry.get<ProjectedPoints>();
            if (!points.empty()) {
                auto pt_size = points.size();
                tile.numPoints += pt_size;
                tile.numSimplified += pt_size;
                tile.features.emplace_back(points, type, feature.tags);
            }
        }
    } else {
        if (feature.geometry.is<ProjectedRings>()) {
            auto const& rings = feature.geometry.get<ProjectedRings>();
            if (noSimplify) {
                if (!rings.empty()) {
                    auto rings_size = rings.size();
                    tile.numPoints += rings_size;
                    tile.numSimplified += rings_size;
                    tile.features.emplace_back(rings, type, feature.tags);
                }
            } else {
                ProjectedRings p_rings;
                // simplify and transform projected coordinates for tile geometry
                for (auto const& ring : rings) {
                    // filter out tiny polylines & polygons
                    if ((type == ProjectedFeatureType::LineString && ring.dist < tolerance) ||
                         (type == ProjectedFeatureType::Polygon && ring.area < sqTolerance)) {
                        tile.numPoints += ring.points.size();
                        continue;
                    }

                    ProjectedRing simplifiedRing;
                    for (auto const& p : ring.points) {
                        // keep points with importance > tolerance
                        if (p.z > sqTolerance) {
                            simplifiedRing.points.emplace_back(p);
                            ++tile.numSimplified;
                        }
                        ++tile.numPoints;
                    }
                    p_rings.emplace_back(std::move(simplifiedRing));
                }

                if (!p_rings.empty()) {
                    tile.features.emplace_back(std::move(p_rings), type, feature.tags);
                }
            }
        }
    }
}

} // namespace geojsonvt
} // namespace mapbox
