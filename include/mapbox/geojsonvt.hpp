#pragma once

#include <mapbox/geojsonvt/convert.hpp>
#include <mapbox/geojsonvt/tile.hpp>
#include <mapbox/geojsonvt/types.hpp>
#include <mapbox/geojsonvt/wrap.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <unordered_map>

namespace mapbox {
namespace geojsonvt {

struct Options {
    // max zoom to preserve detail on
    uint8_t maxZoom = 18;

    // max zoom in the tile index
    uint8_t indexMaxZoom = 5;

    // max number of points per tile in the tile index
    uint32_t indexMaxPoints = 100000;

    // whether to tile solid square tiles further
    bool solidChildren = false;

    // simplification tolerance (higher means simpler)
    double tolerance = 3;

    // tile extent
    uint16_t extent = 4096;

    // tile buffer on each side
    uint16_t buffer = 64;
};

inline uint64_t toID(uint8_t z, uint32_t x, uint32_t y) {
    return (((1 << z) * y + x) * 32) + z;
}

inline vt_point intersectX(const vt_point& a, const vt_point& b, const double x) {
    const double y = (x - a.x) * (b.y - a.y) / (b.x - a.x) + a.y;
    return { x, y, 1.0 };
}

inline vt_point intersectY(const vt_point& a, const vt_point& b, const double y) {
    const double x = (y - a.y) * (b.x - a.x) / (b.y - a.y) + a.x;
    return { x, y, 1.0 };
}

class GeoJSONVT {
public:
    const Options options;

    GeoJSONVT(const geojson_features& features_, const Options& options_ = Options())
        : options(options_) {

        const uint32_t z2 = std::pow(2, options.maxZoom);

        auto converted = convert(features_, options.tolerance / (z2 * options.extent));
        auto features = wrap(converted, double(options.buffer) / options.extent, intersectX);

        splitTile(features, 0, 0, 0);
    }

    std::map<uint8_t, uint32_t> stats;
    uint32_t total = 0;
    std::unordered_map<uint64_t, Tile> tiles;

private:
    void
    splitTile(const vt_features& features, const uint8_t z, const uint32_t x, const uint32_t y) {

        if (features.empty())
            return;

        const double z2 = 1 << z;
        const uint64_t id = toID(z, x, y);

        const auto it = tiles.find(id);

        if (it == tiles.end()) {
            const double tolerance =
                (z == options.maxZoom ? 0 : options.tolerance / (z2 * options.extent));

            tiles.emplace(id, Tile{ features, z, x, y, options.extent, options.buffer, tolerance });
            stats[z] = (stats.count(z) ? stats[z] + 1 : 1);
            total++;
        }

        auto& tile = tiles.find(id)->second;

        // stop tiling if we reached max zoom, or if the tile is too simple
        if (z == options.indexMaxZoom || tile.num_points <= options.indexMaxPoints) {
            tile.source_features = features;
            return;
        }

        const double p = 0.5 * options.buffer / options.extent;
        const auto& min = tile.bbox.min;
        const auto& max = tile.bbox.max;

        const auto left =
            clip(features, (x - p) / z2, (x + 0.5 + p) / z2, 0, intersectX, min.x, max.x);

        if (!left.empty()) {
            splitTile(clip(left, (y - p) / z2, (y + 0.5 + p) / z2, 1, intersectY, min.y, max.y),
                      z + 1, x * 2, y * 2);
            splitTile(clip(left, (y + 0.5 - p) / z2, (y + 1 + p) / z2, 1, intersectY, min.y, max.y),
                      z + 1, x * 2, y * 2 + 1);
        }

        const auto right =
            clip(features, (x + 0.5 - p) / z2, (x + 1 + p) / z2, 0, intersectX, min.x, max.x);

        if (!right.empty()) {
            splitTile(clip(right, (y - p) / z2, (y + 0.5 + p) / z2, 1, intersectY, min.y, max.y),
                      z + 1, x * 2 + 1, y * 2);
            splitTile(
                clip(right, (y + 0.5 - p) / z2, (y + 1 + p) / z2, 1, intersectY, min.y, max.y),
                z + 1, x * 2 + 1, y * 2 + 1);
        }
    }
};

} // namespace geojsonvt
} // namespace mapbox
