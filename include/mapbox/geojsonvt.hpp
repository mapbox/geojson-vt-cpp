#pragma once

#include <mapbox/geojsonvt/convert.hpp>
#include <mapbox/geojsonvt/tile.hpp>
#include <mapbox/geojsonvt/types.hpp>
#include <mapbox/geojsonvt/wrap.hpp>

#include <mapbox/feature.hpp>
#include <mapbox/variant.hpp>

#include <chrono>
#include <cmath>
#include <map>
#include <unordered_map>

namespace mapbox {
namespace geojsonvt {

using geometry = mapbox::geometry::geometry<double>;
using feature = mapbox::feature::feature<double>;
using Update = std::map<mapbox::feature::identifier,
                        std::list<mapbox::util::variant<mapbox::feature::null_value_t, feature>>>;
using feature_collection = mapbox::feature::feature_collection<double>;
using geometry_collection = mapbox::geometry::geometry_collection<double>;
using geojson = mapbox::util::variant<geometry, feature, feature_collection>;

struct ToFeatureCollection {
    feature_collection operator()(const feature_collection& value) const {
        return value;
    }
    feature_collection operator()(const feature& value) const {
        return { value };
    }
    feature_collection operator()(const geometry& value) const {
        return { { value } };
    }
};

struct TileOptions {
    // simplification tolerance (higher means simpler)
    double tolerance = 3;

    // tile extent
    uint16_t extent = 4096;

    // tile buffer on each side
    uint16_t buffer = 64;

    // enable line metrics tracking for LineString/MultiLineString features
    bool lineMetrics = false;
};

struct Options : TileOptions {
    // max zoom to preserve detail on
    uint8_t maxZoom = 18;

    // max zoom in the tile index
    uint8_t indexMaxZoom = 5;

    // max number of points per tile in the tile index
    uint32_t indexMaxPoints = 100000;

    // whether to generate feature ids, overriding existing ids
    bool generateId = false;
};

const std::shared_ptr<const Tile> empty_tile =
        std::make_shared<const Tile>();

inline std::shared_ptr<const Tile> geoJSONToTilePointer(const geojson& geojson_,
                                uint8_t z,
                                uint32_t x,
                                uint32_t y,
                                const TileOptions& options = TileOptions(),
                                bool wrap = false,
                                bool clip = false) {

    const auto features_ = geojson::visit(geojson_, ToFeatureCollection{});
    auto z2 = 1u << z;
    auto tolerance = (options.tolerance / options.extent) / z2;
    uint64_t genId = 0;
    auto features = detail::convert<std::vector, std::list>(features_, tolerance, false, genId, false);
    if (wrap) {
        features = detail::wrap(features, double(options.buffer) / options.extent, options.lineMetrics);
    }
    if (clip || options.lineMetrics) {
        const double p = double(options.buffer) / options.extent;

        const auto left = detail::clip<0>(features, (x - p) / z2, (x + 1 + p) / z2, -1, 2, options.lineMetrics);
        features = detail::clip<1>(left, (y - p) / z2, (y + 1 + p) / z2, -1, 2, options.lineMetrics);
    }
    return detail::InternalTile(features, z, x, y, options.extent, tolerance, options.lineMetrics).getTile();
}

inline const Tile geoJSONToTile(const geojson& geojson_,
                                       uint8_t z,
                                       uint32_t x,
                                       uint32_t y,
                                       const TileOptions& options = TileOptions(),
                                       bool wrap = false,
                                       bool clip = false) {
    return *geoJSONToTilePointer(geojson_, z, x, y, options, wrap, clip);
}

class GeoJSONVT {
public:
    const Options options;
    uint64_t genId = 0;

    GeoJSONVT(const mapbox::feature::feature_collection<double>& features_,
              const Options& options_ = Options())
        : options(options_) {

        const uint32_t z2 = 1u << options.maxZoom;

        auto converted = detail::convert<std::vector, std::list>(features_,
                                                                 (options.tolerance / options.extent) / z2,
                                                                 options.generateId,
                                                                 genId,
                                                                 false);
        auto features = detail::wrap(converted, double(options.buffer) / options.extent, options.lineMetrics);

        splitTile(features, 0, 0, 0);
    }

    GeoJSONVT(const geojson& geojson_, const Options& options_ = Options())
        : GeoJSONVT(geojson::visit(geojson_, ToFeatureCollection{}), options_) {
    }

    std::map<uint8_t, uint32_t> stats;
    uint32_t total = 0;

    const Tile& getTile(const uint8_t z, const uint32_t x_, const uint32_t y) {
        const auto &tile = getTilePointer(z,x_,y);
        return *tile;
    }

    std::shared_ptr<const Tile> getTilePointer(const uint8_t z, const uint32_t x_, const uint32_t y) {

        if (z > options.maxZoom)
            throw std::runtime_error("Requested zoom higher than maxZoom: " + std::to_string(z));

        const uint32_t z2 = 1u << z;
        const uint32_t x = ((x_ % z2) + z2) % z2; // wrap tile x coordinate
        const uint64_t id = detail::InternalTile::toID(z, x, y);

        auto it = tiles.find(id);
        if (it != tiles.end())
            return it->second.getTile();

        it = findParent(z, x, y);

        if (it == tiles.end())
            throw std::runtime_error("Parent tile not found");

        // if we found a parent tile containing the original geometry, we can drill down from it
        const auto& parent = it->second;

        // drill down parent tile up to the requested one
        splitTile(parent.source_features.features, parent.z, parent.x, parent.y, z, x, y);

        it = tiles.find(id);
        if (it != tiles.end())
            return it->second.getTile();

        it = findParent(z, x, y);
        if (it == tiles.end())
            throw std::runtime_error("Parent tile not found");

        return empty_tile;
    }

    const std::unordered_map<uint64_t, detail::InternalTile>& getInternalTiles() const {
        return tiles;
    }

    void updateFeatures(const Update &update) {
        // 1. erase all updates keys from all cached tiles
        for (auto &t: tiles) {
            auto &tile = t.second;
            for (auto u: update) {
                tile.removeFeature(u.first);
            }
        }
        // 2. Add all features != nullopt
        // 2.1 Add features to a feature collection to feed to detail::convert, and calculate the bounding box.
        mapbox::feature::feature_collection<double, std::list> newFeatures;
        for (auto u: update) {
            for (auto f: u.second) {
                if (!f.is<mapbox::feature::null_value_t>()) {
                    newFeatures.push_back(f.get_unchecked<feature>());

                }
            }
        }
        uint32_t z2 = 1u << options.maxZoom;
        auto converted = detail::convert(newFeatures,
                                         (options.tolerance / options.extent) / z2,
                                         options.generateId,
                                         genId,
                                         true);
        auto features = detail::wrap(converted, double(options.buffer) / options.extent, options.lineMetrics);

        // Calculate the bbox to help with trivial accept/reject
        mapbox::geometry::box<double> newDataBbox = { { 2, 1 }, { -1, 0 } };
        for (const auto &f: features) {
            detail::InternalTile::extendBox(newDataBbox, f.bbox);
        }
        const auto& min = newDataBbox.min;
        const auto& max = newDataBbox.max;

        // 2.2 For all already generated tiles, add clipped newFeatures to those tiles
        for (auto &t: tiles) {
            auto &key = t.first;
            auto &tile = t.second;
            const auto coords = detail::InternalTile::fromID(key);
            const auto x = std::get<0>(coords);
            const auto y = std::get<1>(coords);
            const auto z = std::get<2>(coords);
            z2 = 1u << z;
            const double p = 0.5 * options.buffer / options.extent;

            const auto tileLeft = (x - p) / z2;
            const auto tileRight = (x + 1 + p) / z2;
            const auto tileTop = (y - p) / z2;
            const auto tileBottom = (y + 1 + p) / z2;

            const auto xClipped = detail::clip<0>(features, tileLeft, tileRight, min.x, max.x, options.lineMetrics);
            const auto xyClipped = detail::clip<1>(xClipped, tileTop, tileBottom, min.y, max.y, options.lineMetrics);

            if (!xyClipped.empty()) {
                tile.insertFeatures(xyClipped);
            }
        }
    }

private:
    std::unordered_map<uint64_t, detail::InternalTile> tiles;

    std::unordered_map<uint64_t, detail::InternalTile>::iterator
    findParent(const uint8_t z, const uint32_t x, const uint32_t y) {
        uint8_t z0 = z;
        uint32_t x0 = x;
        uint32_t y0 = y;

        const auto end = tiles.end();
        auto parent = end;

        while ((parent == end) && (z0 != 0)) {
            z0--;
            x0 = x0 / 2;
            y0 = y0 / 2;
            parent = tiles.find(detail::InternalTile::toID(z0, x0, y0));
        }

        return parent;
    }

    // splits features from a parent tile to sub-tiles.
    // z, x, and y are the coordinates of the parent tile
    // cz, cx, and cy are the coordinates of the target tile
    //
    // If no target tile is specified, splitting stops when we reach the maximum
    // zoom or the number of points is low as specified in the options.
    void splitTile(const detail::vt_features& features,
                   const uint8_t z,
                   const uint32_t x,
                   const uint32_t y,
                   const uint8_t cz = 0,
                   const uint32_t cx = 0,
                   const uint32_t cy = 0) {

        const double z2 = 1u << z;
        uint64_t id = detail::InternalTile::toID(z, x, y);

        auto it = tiles.find(id);

        if (it == tiles.end()) {
            const double tolerance =
                (z == options.maxZoom ? 0 : options.tolerance / (z2 * options.extent));



            it = tiles
                     .emplace(std::piecewise_construct, std::make_tuple(id),
                              std::make_tuple(features, z, x, y, options.extent, tolerance, options.lineMetrics))
                     .first;
            stats[z] = (stats.count(z) ? stats[z] + 1 : 1);
            total++;
            // printf("tile z%i-%i-%i\n", z, x, y);
        }

        auto& tile = it->second;

        if (features.empty())
            return;

        // if it's the first-pass tiling
        if (cz == 0u) {
            // stop tiling if we reached max zoom, or if the tile is too simple
            if (z == options.indexMaxZoom || tile.num_points <= options.indexMaxPoints) {
                return;
            }

        } else { // drilldown to a specific tile;
            // stop tiling if we reached base zoom
            if (z == options.maxZoom) {
                tile.source_features.clear();
                return;
            }

            // stop tiling if it's our target tile zoom
            if (z == cz) {
                return;
            }

            // stop tiling if it's not an ancestor of the target tile
            const double m = 1u << (cz - z);
            if (x != static_cast<uint32_t>(std::floor(cx / m)) ||
                y != static_cast<uint32_t>(std::floor(cy / m))) {
                return;
            }
        }

        const double p = 0.5 * options.buffer / options.extent;
        const auto& min = tile.bbox.min;
        const auto& max = tile.bbox.max;

        // k1 and k2, just like vt_features, are in mercator coordinates.
        const auto left = detail::clip<0>(features, (x - p) / z2, (x + 0.5 + p) / z2, min.x, max.x, options.lineMetrics);

        // tl
        splitTile(detail::clip<1>(left, (y - p) / z2, (y + 0.5 + p) / z2, min.y, max.y, options.lineMetrics), z + 1,
                  x * 2, y * 2, cz, cx, cy);
        // bl
        splitTile(detail::clip<1>(left, (y + 0.5 - p) / z2, (y + 1 + p) / z2, min.y, max.y, options.lineMetrics), z + 1,
                  x * 2, y * 2 + 1, cz, cx, cy);

        const auto right =
            detail::clip<0>(features, (x + 0.5 - p) / z2, (x + 1 + p) / z2, min.x, max.x, options.lineMetrics);

        // tr
        splitTile(detail::clip<1>(right, (y - p) / z2, (y + 0.5 + p) / z2, min.y, max.y, options.lineMetrics), z + 1,
                  x * 2 + 1, y * 2, cz, cx, cy);
        // br
        splitTile(detail::clip<1>(right, (y + 0.5 - p) / z2, (y + 1 + p) / z2, min.y, max.y, options.lineMetrics), z + 1,
                  x * 2 + 1, y * 2 + 1, cz, cx, cy);

        // if we sliced further down, no need to keep source geometry
        tile.source_features.clear();
    }
};

} // namespace geojsonvt
} // namespace mapbox
