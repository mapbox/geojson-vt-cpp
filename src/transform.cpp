#include <mapbox/geojsonvt/transform.hpp>

#include <cmath>

namespace mapbox {
namespace geojsonvt {

TilePoint Transform::transformPoint(
    const ProjectedPoint& p, uint16_t extent, uint32_t z2, uint32_t tx, uint32_t ty) {

    int16_t x = ::round(extent * (p.x * z2 - tx));
    int16_t y = ::round(extent * (p.y * z2 - ty));

    return TilePoint(x, y);
}

const Tile& Transform::transformTile(Tile& tile, uint16_t extent) {
    if (tile.transformed) {
        return tile;
    }

    const uint32_t z2 = tile.z2;
    const uint32_t tx = tile.tx;
    const uint32_t ty = tile.ty;

    for (auto& feature : tile.features) {
        const auto& geom = feature.geometry;
        const auto type = feature.type;

        if (type == TileFeatureType::Point) {
            auto& tileGeom = feature.tileGeometry.get<TilePoints>();
            for (const auto& pt : geom.get<ProjectedPoints>()) {
                tileGeom.push_back(transformPoint(pt, extent, z2, tx, ty));
            }

        } else {
            feature.tileGeometry.set<TileRings>();
            auto& tileGeom = feature.tileGeometry.get<TileRings>();
            for (const auto& r : geom.get<ProjectedRings>()) {
                TilePoints ring;
                for (const auto& p : r.points) {
                    ring.push_back(transformPoint(p, extent, z2, tx, ty));
                }
                tileGeom.push_back(std::move(ring));
            }
        }
    }

    tile.transformed = true;

    return tile;
}

} // namespace geojsonvt
} // namespace mapbox
