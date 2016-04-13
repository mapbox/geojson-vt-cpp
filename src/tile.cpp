#include <mapbox/geojsonvt/tile.hpp>

namespace mapbox {
namespace geojsonvt {

Tile Tile::createTile(std::vector<ProjectedFeature>& features,
                      uint32_t z2,
                      uint32_t tx,
                      uint32_t ty,
                      double tolerance,
                      bool noSimplify) {
    Tile tile;

    tile.z2 = z2;
    tile.tx = tx;
    tile.ty = ty;

    for (const auto& feature : features) {
        tile.numFeatures++;
        addFeature(tile, feature, tolerance, noSimplify);

        const auto& min = feature.min;
        const auto& max = feature.max;

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
                      const ProjectedFeature& feature,
                      double tolerance,
                      bool noSimplify) {

    const ProjectedFeatureType type = feature.type;
    ProjectedGeometry simplified;
    const double sqTolerance = tolerance * tolerance;

    if (type == ProjectedFeatureType::Point) {
        for (auto& p : feature.geometry.get<ProjectedPoints>()) {
            simplified.get<ProjectedPoints>().push_back(p);
            tile.numPoints++;
            tile.numSimplified++;
        }
        if (simplified.get<ProjectedPoints>().empty()) {
            return;
        }

    } else {
        simplified.set<ProjectedRings>();

        // simplify and transform projected coordinates for tile geometry
        for (auto& ring : feature.geometry.get<ProjectedRings>()) {
            // filter out tiny polylines & polygons
            if (!noSimplify &&
                ((type == ProjectedFeatureType::LineString && ring.dist < tolerance) ||
                 (type == ProjectedFeatureType::Polygon && ring.area < sqTolerance))) {

                tile.numPoints += ring.points.size();
                continue;
            }

            ProjectedRing simplifiedRing;

            for (auto& p : ring.points) {
                // keep points with importance > tolerance
                if (noSimplify || p.z > sqTolerance) {
                    simplifiedRing.points.push_back(p);
                    tile.numSimplified++;
                }
                tile.numPoints++;
            }

            simplified.get<ProjectedRings>().push_back(simplifiedRing);
        }

        if (simplified.get<ProjectedRings>().empty()) {
            return;
        }
    }

    tile.features.push_back(TileFeature(simplified, type, feature.tags));
}

} // namespace geojsonvt
} // namespace mapbox
