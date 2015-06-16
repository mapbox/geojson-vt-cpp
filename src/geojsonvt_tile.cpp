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

    for (size_t i = 0; i < features.size(); ++i) {
        tile.numFeatures++;
        addFeature(tile, features[i], tolerance, noSimplify);

        const auto& min = features[i].min;
        const auto& max = features[i].max;

        if (min.x < tile.min.x) tile.min.x = min.x;
        if (min.y < tile.min.y) tile.min.y = min.y;
        if (max.x > tile.max.x) tile.max.x = max.x;
        if (max.y > tile.max.y) tile.max.y = max.y;
    }

    return std::move(tile);
}

void Tile::addFeature(Tile& tile,
                      ProjectedFeature& feature,
                      double tolerance,
                      bool noSimplify) {

    ProjectedGeometryContainer* geom = &(feature.geometry.get<ProjectedGeometryContainer>());
    ProjectedFeatureType type = feature.type;
    std::vector<ProjectedGeometry> simplified;
    double sqTolerance = tolerance * tolerance;
    ProjectedGeometryContainer ring;

    if (type == ProjectedFeatureType::Point) {
        for (size_t i = 0; i < geom->members.size(); ++i) {
            ProjectedPoint* p = &(geom->members[i].get<ProjectedPoint>());
            simplified.push_back(*p);
            tile.numPoints++;
            tile.numSimplified++;
        }
    } else {
        for (size_t i = 0; i < geom->members.size(); ++i) {
            ring = geom->members[i].get<ProjectedGeometryContainer>();

            if (!noSimplify &&
                ((type == ProjectedFeatureType::LineString && ring.dist < tolerance) ||
                 (type == ProjectedFeatureType::Polygon && ring.area < sqTolerance))) {
                tile.numPoints += ring.members.size();
                continue;
            }

            ProjectedGeometryContainer simplifiedRing;

            for (size_t j = 0; j < ring.members.size(); ++j) {
                ProjectedPoint* p = &(ring.members[j].get<ProjectedPoint>());
                if (noSimplify || p->z > sqTolerance) {
                    simplifiedRing.members.push_back(*p);
                    tile.numSimplified++;
                }
                tile.numPoints++;
            }

            simplified.push_back(simplifiedRing);
        }
    }

    if (simplified.size()) {
        tile.features.push_back(TileFeature(simplified, type, Tags(feature.tags)));
    }
}

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
