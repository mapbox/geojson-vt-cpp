#include <mapbox/geojsonvt/geojsonvt_tile.hpp>

namespace mapbox {
namespace util {
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

        if (min.x < tile.min.x)
            tile.min.x = min.x;
        if (min.y < tile.min.y)
            tile.min.y = min.y;
        if (max.x > tile.max.x)
            tile.max.x = max.x;
        if (max.y > tile.max.y)
            tile.max.y = max.y;
    }

    return std::move(tile);
}

void
Tile::addFeature(Tile& tile, const ProjectedFeature& feature, double tolerance, bool noSimplify) {
    const ProjectedGeometryContainer& geom = feature.geometry.get<ProjectedGeometryContainer>();
    const ProjectedFeatureType type = feature.type;
    std::vector<ProjectedGeometry> simplified;
    const double sqTolerance = tolerance * tolerance;

    if (type == ProjectedFeatureType::Point) {
        for (auto& member : geom.members) {
            simplified.push_back(member.get<ProjectedPoint>());
            tile.numPoints++;
            tile.numSimplified++;
        }
    } else {
        // simplify and transform projected coordinates for tile geometry
        for (auto& member : geom.members) {
            auto& ring = member.get<ProjectedGeometryContainer>();

            // filter out tiny polylines & polygons
            if (!noSimplify &&
                ((type == ProjectedFeatureType::LineString && ring.dist < tolerance) ||
                 (type == ProjectedFeatureType::Polygon && ring.area < sqTolerance))) {
                tile.numPoints += ring.members.size();
                continue;
            }

            ProjectedGeometryContainer simplifiedRing;

            for (auto& ringMember : ring.members) {
                auto& p = ringMember.get<ProjectedPoint>();
                // keep points with importance > tolerance
                if (noSimplify || p.z > sqTolerance) {
                    simplifiedRing.members.push_back(p);
                    tile.numSimplified++;
                }
                tile.numPoints++;
            }

            simplified.push_back(simplifiedRing);
        }
    }

    if (!simplified.empty()) {
        tile.features.push_back(TileFeature(simplified, type, feature.tags));
    }
}

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
