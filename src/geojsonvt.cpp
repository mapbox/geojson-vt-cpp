#include <mapbox/geojsonvt/geojsonvt.hpp>
#include <mapbox/geojsonvt/geojsonvt_clip.hpp>
#include <mapbox/geojsonvt/geojsonvt_convert.hpp>
#include <mapbox/geojsonvt/geojsonvt_util.hpp>
#include <mapbox/geojsonvt/geojsonvt_wrap.hpp>

#include <stack>
#include <cmath>

namespace mapbox {
namespace util {
namespace geojsonvt {

std::unordered_map<std::string, clock_t> Time::activities;

#pragma mark - GeoJSONVT

std::vector<ProjectedFeature>
GeoJSONVT::convertFeatures(const std::string& data, uint8_t maxZoom, double tolerance, bool debug) {
    if (debug) {
        Time::time("preprocess data");
    }

    uint32_t z2 = 1 << maxZoom; // 2^z

    JSDocument deserializedData;
    deserializedData.Parse<0>(data.c_str());

    if (deserializedData.HasParseError()) {
        throw std::runtime_error("Invalid GeoJSON");
    }

    const uint16_t extent = 4096;

    std::vector<ProjectedFeature> features =
        Convert::convert(deserializedData, tolerance / (z2 * extent));

    if (debug) {
        Time::timeEnd("preprocess data");
    }

    return features;
}

const Tile GeoJSONVT::emptyTile {};

GeoJSONVT::GeoJSONVT(std::vector<ProjectedFeature> features_,
                     uint8_t maxZoom_,
                     uint8_t indexMaxZoom_,
                     uint32_t indexMaxPoints_,
                     bool solidChildren_,
                     double tolerance_,
                     bool debug_)
    : maxZoom(maxZoom_),
      indexMaxZoom(indexMaxZoom_),
      indexMaxPoints(indexMaxPoints_),
      solidChildren(solidChildren_),
      tolerance(tolerance_),
      debug(debug_) {
    if (debug) {
        printf("index: maxZoom: %d, maxPoints: %d", indexMaxZoom, indexMaxPoints);
        Time::time("generate tiles");
    }

    features_ = Wrap::wrap(features_, double(buffer) / extent, intersectX);

    // start slicing from the top tile down
    if (!features_.empty()) {
        splitTile(features_, 0, 0, 0);
    }

    if (debug) {
        if (!features_.empty()) {
            printf("features: %i, points: %i\n", tiles[0].numFeatures, tiles[0].numPoints);
        }
        Time::timeEnd("generate tiles");
        printf("tiles generated: %i {\n", total);
        for (const auto& pair : stats) {
            printf("    z%i: %i\n", pair.first, pair.second);
        }
        printf("}\n");
    }
}

void GeoJSONVT::splitTile(std::vector<ProjectedFeature> features_,
                          uint8_t z_,
                          uint32_t x_,
                          uint32_t y_,
                          uint8_t cz,
                          uint32_t cx,
                          uint32_t cy) {
    std::stack<FeatureStackItem> stack;
    stack.emplace(features_, z_, x_, y_);

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
        double tileTolerance = (z == maxZoom ? 0 : tolerance / (z2 * extent));

        if (!tile) {
            if (debug) {
                Time::time("creation");
            }

            tiles[id] =
                std::move(Tile::createTile(features, z2, x, y, tileTolerance, (z == maxZoom)));
            tile = &tiles[id];

            if (debug) {
                printf("tile z%i-%i-%i (features: %i, points: %i, simplified: %i\n", z, x, y,
                       tile->numFeatures, tile->numPoints, tile->numSimplified);
                Time::timeEnd("creation");

                uint8_t key = z;
                stats[key] = (stats.count(key) ? stats[key] + 1 : 1);
            }
            total++;
        }

        // save reference to original geometry in tile so that we can drill down later if we stop
        // now
        tile->source = std::vector<ProjectedFeature>(features);

        // stop tiling if the tile is solid clipped square
        if (!solidChildren && isClippedSquare(*tile, extent, buffer)) continue;

        // if it's the first-pass tiling
        if (!cz) {
            // stop tiling if we reached max zoom, or if the tile is too simple
            if (z == indexMaxZoom || tile->numPoints <= indexMaxPoints)
                continue;

            // if a drilldown to a specific tile
        } else {
            // stop tiling if we reached base zoom or our target tile zoom
            if (z == maxZoom || z == cz)
                continue;

            // stop tiling if it's not an ancestor of the target tile
            const auto m = 1 << (cz - z);
            if (x != std::floor(cx / m) && y != std::floor(cy / m))
                continue;
        }

        // if we slice further down, no need to keep source geometry
        tile->source = {};

        if (debug) {
            Time::time("clipping");
        }

        const double k1 = 0.5 * buffer / extent;
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

        if (debug) {
            Time::timeEnd("clipping");
        }

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
}

const Tile& GeoJSONVT::getTile(uint8_t z, uint32_t x, uint32_t y) {
    std::lock_guard<std::mutex> lock(mtx);

    const uint32_t z2 = 1 << z;
    x = ((x % z2) + z2) % z2; // wrap tile x coordinate

    const uint64_t id = toID(z, x, y);
    if (tiles.count(id)) {
        return transformTile(tiles.find(id)->second, extent);
    }

    if (debug) {
        printf("drilling down to z%i-%i-%i\n", z, x, y);
    }

    uint8_t z0 = z;
    uint32_t x0 = x;
    uint32_t y0 = y;
    Tile* parent = nullptr;

    while (!parent && z0) {
        z0--;
        x0 = x0 / 2;
        y0 = y0 / 2;
        const uint64_t checkID = toID(z0, x0, y0);
        if (tiles.count(checkID)) {
            parent = &tiles[checkID];
        }
    }

    if (!parent) {
        return emptyTile;
    }

    if (debug) {
        printf("found parent tile z%i-%i-%i\n", z0, x0, y0);
    }

    // if we found a parent tile containing the original geometry, we can drill down from it
    if (parent && !parent->source.empty()) {
        if (isClippedSquare(*parent, extent, buffer)) {
            return transformTile(*parent, extent);
        }

        if (debug) {
            Time::time("drilling down");
        }

        splitTile(parent->source, z0, x0, y0, z, x, y);

        if (debug) {
            Time::timeEnd("drilling down");
        }
    }

    if (tiles.find(id) == tiles.end()) {
        return emptyTile;
    }

    return transformTile(tiles[id], extent);
}

const std::map<uint64_t, Tile>& GeoJSONVT::getAllTiles() const {
    return tiles;
}

uint64_t GeoJSONVT::getTotal() const {
    return total;
}

const Tile& GeoJSONVT::transformTile(Tile& tile, uint16_t extent) {
    if (tile.transformed) {
        return tile;
    }

    const uint32_t z2 = tile.z2;
    const uint32_t tx = tile.tx;
    const uint32_t ty = tile.ty;

    for (size_t i = 0; i < tile.features.size(); i++) {
        auto& feature = tile.features[i];
        const auto& geom = feature.geometry;
        const auto type = feature.type;

        if (type == TileFeatureType::Point) {
            for (const auto& pt : geom) {
                feature.tileGeometry.push_back(
                    transformPoint(pt.get<ProjectedPoint>(), extent, z2, tx, ty));
            }

        } else {
            for (const auto& r : geom) {
                TileRing ring;
                for (const auto& pt : r.get<ProjectedGeometryContainer>().members) {
                    ring.points.push_back(
                        transformPoint(pt.get<ProjectedPoint>(), extent, z2, tx, ty));
                }
                feature.tileGeometry.emplace_back(std::move(ring));
            }
        }
    }

    tile.transformed = true;

    return tile;
}

TilePoint GeoJSONVT::transformPoint(
    const ProjectedPoint& p, uint16_t extent, uint32_t z2, uint32_t tx, uint32_t ty) {

    int16_t x = std::round(extent * (p.x * z2 - tx));
    int16_t y = std::round(extent * (p.y * z2 - ty));

    return TilePoint(x, y);
}

uint64_t GeoJSONVT::toID(uint8_t z, uint32_t x, uint32_t y) {
    return (((1 << z) * y + x) * 32) + z;
}

ProjectedPoint GeoJSONVT::intersectX(const ProjectedPoint& a, const ProjectedPoint& b, double x) {
    double r1 = x;
    double r2 = (x - a.x) * (b.y - a.y) / (b.x - a.x) + a.y;
    double r3 = 1;

    return ProjectedPoint(r1, r2, r3);
}

ProjectedPoint GeoJSONVT::intersectY(const ProjectedPoint& a, const ProjectedPoint& b, double y) {
    double r1 = (y - a.y) * (b.x - a.x) / (b.y - a.y) + a.x;
    double r2 = y;
    double r3 = 1;

    return ProjectedPoint(r1, r2, r3);
}

// checks whether a tile is a whole-area fill after clipping; if it is, there's no sense slicing it
// further
bool GeoJSONVT::isClippedSquare(Tile& tile, const uint16_t extent, const uint8_t buffer) {
    const auto& features = tile.source;
    if (features.size() != 1) {
        return false;
    }

    const auto& feature = features.front();
    const auto& geometries = feature.geometry.get<ProjectedGeometryContainer>();
    if (feature.type != ProjectedFeatureType::Polygon || geometries.members.size() > 1) {
        return false;
    }

    const auto& geometry = geometries.members.front().get<ProjectedGeometryContainer>();

    if (geometry.members.size() != 5) {
        return false;
    }

    for (const auto& pt : geometry.members) {
        auto p = transformPoint(pt.get<ProjectedPoint>(), extent, tile.z2, tile.tx, tile.ty);
        if ((p.x != -buffer && p.x != extent + buffer) ||
            (p.y != -buffer && p.y != extent + buffer)) {
            return false;
        }
    }

    return true;
}

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
