#include <mapbox/geojsonvt.hpp>
#include <mapbox/geojsonvt/clip.hpp>
#include <mapbox/geojsonvt/convert.hpp>
#include <mapbox/geojsonvt/transform.hpp>
#include <mapbox/geojsonvt/wrap.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <utility>

namespace mapbox {
namespace geojsonvt {

#pragma mark - GeoJSONVT

const Tile GeoJSONVT::emptyTile{};

namespace {

uint64_t toID(uint8_t z, uint32_t x, uint32_t y) {
    return (((1 << z) * y + x) * 32) + z;
}

ProjectedPoint intersectX(const ProjectedPoint& a, const ProjectedPoint& b, double x) {
    double y = (x - a.x) * (b.y - a.y) / (b.x - a.x) + a.y;
    return ProjectedPoint(x, y, 1.0);
}

ProjectedPoint intersectY(const ProjectedPoint& a, const ProjectedPoint& b, double y) {
    double x = (y - a.y) * (b.x - a.x) / (b.y - a.y) + a.x;
    return ProjectedPoint(x, y, 1.0);
}

// checks whether a tile is a whole-area fill after clipping; if it is, there's no sense slicing it
// further
bool isClippedSquare(Tile& tile, const uint16_t extent, const uint8_t buffer) {
    const auto& features = tile.source;
    if (features.size() != 1) {
        return false;
    }

    const auto& feature = features.front();
    if (feature.type != ProjectedFeatureType::Polygon) {
        return false;
    }
    const auto& rings = feature.geometry.get<ProjectedRings>();
    if (rings.size() > 1) {
        return false;
    }

    const auto& ring = rings.front();

    if (ring.points.size() != 5) {
        return false;
    }

    for (const auto& pt : ring.points) {
        auto p = Transform::transformPoint(pt, extent, tile.z2, tile.tx, tile.ty);
        if ((p.x != -buffer && p.x != extent + buffer) ||
            (p.y != -buffer && p.y != extent + buffer)) {
            return false;
        }
    }

    return true;
}

class Timer {
    using Clock = std::chrono::steady_clock;

    Clock::time_point start = Clock::now();

public:
    void report(const std::string& name) {
        Clock::duration duration = Clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::microseconds>(duration);
        printf("%s: %.2fms\n", name.c_str(), double(ms.count()) / 1000);
        start += duration;
    }
};

struct FeatureStackItem {
    std::vector<ProjectedFeature> features;
    uint8_t z;
    uint32_t x;
    uint32_t y;

    FeatureStackItem(std::vector<ProjectedFeature> features_, uint8_t z_, uint32_t x_, uint32_t y_)
        : features(std::move(features_)), z(z_), x(x_), y(y_) {
    }
};
} // namespace

GeoJSONVT::GeoJSONVT(std::vector<ProjectedFeature> features_, Options options_)
    : options(std::move(options_)) {

#ifdef DEBUG
    printf("index: maxZoom: %d, maxPoints: %d\n", options.indexMaxZoom, options.indexMaxPoints);
    Timer timer;
#endif

    features_ = Wrap::wrap(features_, double(options.buffer) / options.extent, intersectX);

    // start slicing from the top tile down
    if (!features_.empty()) {
        splitTile(features_, 0, 0, 0);
    }

#ifdef DEBUG
    timer.report("generate tiles");
    if (!features_.empty()) {
        printf("features: %i, points: %i\n", tiles[0].numFeatures, tiles[0].numPoints);
    }
    printf("tiles generated: %i {\n", static_cast<int>(total));
    for (const auto& pair : stats) {
        printf("    z%i: %i\n", pair.first, pair.second);
    }
    printf("}\n");
#endif
}

const Tile& GeoJSONVT::getTile(uint8_t z, uint32_t x, uint32_t y) {
    const uint32_t z2 = 1 << z;
    x = ((x % z2) + z2) % z2; // wrap tile x coordinate

    uint64_t id = toID(z, x, y);
    if (tiles.count(id) != 0u) {
        return Transform::transformTile(tiles.find(id)->second, options.extent);
    }

#ifdef DEBUG
    printf("drilling down to z%i-%i-%i\n", z, x, y);
#endif

    uint8_t z0 = z;
    uint32_t x0 = x;
    uint32_t y0 = y;
    Tile* parent = nullptr;

    while ((parent == nullptr) && (z0 != 0u)) {
        z0--;
        x0 = x0 / 2;
        y0 = y0 / 2;
        const uint64_t checkID = toID(z0, x0, y0);
        if (tiles.count(checkID) != 0u) {
            parent = &tiles[checkID];
        }
    }

    if (parent == nullptr || parent->source.empty()) {
        return emptyTile;
    }

#ifdef DEBUG
    printf("found parent tile z%i-%i-%i\n", z0, x0, y0);
#endif

    // if we found a parent tile containing the original geometry, we can drill down from it

    // parent tile is a solid clipped square, return it instead since it's identical
    if (isClippedSquare(*parent, options.extent, options.buffer)) {
        return Transform::transformTile(*parent, options.extent);
    }

#ifdef DEBUG
    Timer timer;
#endif
    uint8_t solidZ = splitTile(parent->source, z0, x0, y0, z, x, y);
#ifdef DEBUG
    timer.report("drilling down");
#endif

    // one of the parent tiles was a solid clipped square
    if (solidZ != 0u) {
        const auto m = 1 << (z - solidZ);
        id = toID(solidZ, std::floor(x / m), std::floor(y / m));
    }

    if (tiles.find(id) == tiles.end()) {
        return emptyTile;
    }

    return Transform::transformTile(tiles[id], options.extent);
}

const std::map<uint64_t, Tile>& GeoJSONVT::getAllTiles() const {
    return tiles;
}

uint64_t GeoJSONVT::getTotal() const {
    return total;
}

std::vector<ProjectedFeature> GeoJSONVT::convertFeatures(const std::string& data, Options options) {
#ifdef DEBUG
    Timer timer;
#endif

    uint32_t z2 = 1 << options.maxZoom; // 2^z

    JSDocument deserializedData;
    deserializedData.Parse<0>(data.c_str());

    if (deserializedData.HasParseError()) {
        throw std::runtime_error("Invalid GeoJSON");
    }

    std::vector<ProjectedFeature> features =
        Convert::convert(deserializedData, options.tolerance / (z2 * options.extent));

#ifdef DEBUG
    timer.report("preprocess data");
#endif

    return features;
}

uint8_t GeoJSONVT::splitTile(std::vector<ProjectedFeature> features_,
                             uint8_t z_,
                             uint32_t x_,
                             uint32_t y_,
                             uint8_t cz,
                             uint32_t cx,
                             uint32_t cy) {
    std::stack<FeatureStackItem> stack;
    stack.emplace(features_, z_, x_, y_);

    uint8_t solidZ = 0u;

    while (!stack.empty()) {
        FeatureStackItem set = stack.top();
        stack.pop();
        std::vector<ProjectedFeature> features = std::move(set.features);
        uint8_t z = set.z;
        uint32_t x = set.x;
        uint32_t y = set.y;

        uint32_t z2 = 1 << z;
        const uint64_t id = toID(z, x, y);
        Tile* tile = [&]() {
            const auto it = tiles.find(id);
            return it != tiles.end() ? &it->second : nullptr;
        }();
        double tileTolerance =
            (z == options.maxZoom ? 0 : options.tolerance / (z2 * options.extent));

        if (tile == nullptr) {
#ifdef DEBUG
            Timer timer;
#endif

            tiles[id] = std::move(
                Tile::createTile(features, z2, x, y, tileTolerance, (z == options.maxZoom)));
            tile = &tiles[id];

#ifdef DEBUG
            printf("tile z%i-%i-%i (features: %i, points: %i, simplified: %i)\n", z, x, y,
                   tile->numFeatures, tile->numPoints, tile->numSimplified);
            timer.report("creation");

            uint8_t key = z;
            stats[key] = (stats.count(key) ? stats[key] + 1 : 1);
#endif

            total++;
        }

        // save reference to original geometry in tile so that we can drill down later if we stop
        // now
        tile->source = std::vector<ProjectedFeature>(features);

        // if it's the first-pass tiling
        if (cz == 0u) {
            // stop tiling if we reached max zoom, or if the tile is too simple
            if (z == options.indexMaxZoom || tile->numPoints <= options.indexMaxPoints) {
                continue;
            }

            // if a drilldown to a specific tile
        } else {
            // stop tiling if we reached base zoom or our target tile zoom
            if (z == options.maxZoom || z == cz) {
                continue;
            }

            // stop tiling if it's not an ancestor of the target tile
            const auto m = 1 << (cz - z);
            if (x != std::floor(cx / m) || y != std::floor(cy / m)) {
                continue;
            }
        }

        // stop tiling if the tile is solid clipped square
        if (!options.solidChildren && isClippedSquare(*tile, options.extent, options.buffer)) {
            if (cz != 0u)
                solidZ = z; // and remember the zoom if we're drilling down
            continue;
        }

        // if we slice further down, no need to keep source geometry
        tile->source = {};

#ifdef DEBUG
        Timer timer;
#endif

        const double k1 = 0.5 * options.buffer / options.extent;
        const double k2 = 0.5 - k1;
        const double k3 = 0.5 + k1;
        const double k4 = 1 + k1;

        std::vector<ProjectedFeature> tl;
        std::vector<ProjectedFeature> bl;
        std::vector<ProjectedFeature> tr;
        std::vector<ProjectedFeature> br;

        const auto left =
            Clip::clip(features, z2, x - k1, x + k3, 0, intersectX, tile->min.x, tile->max.x);
        const auto right =
            Clip::clip(features, z2, x + k2, x + k4, 0, intersectX, tile->min.x, tile->max.x);

        if (!left.empty()) {
            tl = Clip::clip(left, z2, y - k1, y + k3, 1, intersectY, tile->min.y, tile->max.y);
            bl = Clip::clip(left, z2, y + k2, y + k4, 1, intersectY, tile->min.y, tile->max.y);
        }

        if (!right.empty()) {
            tr = Clip::clip(right, z2, y - k1, y + k3, 1, intersectY, tile->min.y, tile->max.y);
            br = Clip::clip(right, z2, y + k2, y + k4, 1, intersectY, tile->min.y, tile->max.y);
        }

#ifdef DEBUG
        timer.report("clipping");
#endif

        if (!tl.empty()) {
            stack.emplace(std::move(tl), z + 1, x * 2, y * 2);
        }
        if (!bl.empty()) {
            stack.emplace(std::move(bl), z + 1, x * 2, y * 2 + 1);
        }
        if (!tr.empty()) {
            stack.emplace(std::move(tr), z + 1, x * 2 + 1, y * 2);
        }
        if (!br.empty()) {
            stack.emplace(std::move(br), z + 1, x * 2 + 1, y * 2 + 1);
        }
    }

    return solidZ;
}

} // namespace geojsonvt
} // namespace mapbox
