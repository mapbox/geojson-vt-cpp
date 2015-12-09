#ifndef MAPBOX_GEOJSONVT
#define MAPBOX_GEOJSONVT

#include "geojsonvt/types.hpp"
#include "geojsonvt/tile.hpp"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace mapbox {
namespace geojsonvt {

struct Options {
    // max zoom to preserve detail on
    uint8_t maxZoom = 14;

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
    uint8_t buffer = 64;
};

class __attribute__((visibility("default"))) GeoJSONVT {
public:
    static const Tile emptyTile;

    GeoJSONVT(const std::string&, Options = Options());

    const Tile& getTile(uint8_t z, uint32_t x, uint32_t y);

    const std::map<uint64_t, Tile>& getAllTiles() const;

    // returns the total number of tiles generated until now.
    uint64_t getTotal() const;

private:
    std::vector<ProjectedFeature> convertFeatures(const std::string& data);

    void splitTile(std::vector<ProjectedFeature> features_,
                   uint8_t z_,
                   uint32_t x_,
                   uint32_t y_,
                   uint8_t cz = 0,
                   uint32_t cx = 0,
                   uint32_t cy = 0);

    static TilePoint
    transformPoint(const ProjectedPoint& p, uint16_t extent, uint32_t z2, uint32_t tx, uint32_t ty);
    static const Tile& transformTile(Tile&, uint16_t extent);

    static uint64_t toID(uint8_t z, uint32_t x, uint32_t y);

    static ProjectedPoint intersectX(const ProjectedPoint& a, const ProjectedPoint& b, double x);

    static ProjectedPoint intersectY(const ProjectedPoint& a, const ProjectedPoint& b, double y);

    static bool isClippedSquare(Tile& tile, uint16_t extent, uint8_t buffer);

    struct FeatureStackItem {
        std::vector<ProjectedFeature> features;
        uint8_t z;
        uint32_t x;
        uint32_t y;

        FeatureStackItem(std::vector<ProjectedFeature> features_,
                         uint8_t z_,
                         uint32_t x_,
                         uint32_t y_)
            : features(features_), z(z_), x(x_), y(y_) {
        }
    };

private:
    const Options options;
    std::map<uint64_t, Tile> tiles;
    std::map<uint8_t, uint16_t> stats;
    uint64_t total = 0;
};

} // namespace geojsonvt
} // namespace mapbox

#endif // MAPBOX_GEOJSONVT
