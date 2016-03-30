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

class __attribute__((visibility("default"))) GeoJSONVT {
public:
    static const Tile emptyTile;

    GeoJSONVT(std::vector<ProjectedFeature> features_, Options = Options());

    static std::vector<ProjectedFeature> convertFeatures(const std::string& data,
                                                         Options options = Options());

    const Tile& getTile(uint8_t z, uint32_t x, uint32_t y);

    const std::map<uint64_t, Tile>& getAllTiles() const;

    // returns the total number of tiles generated until now.
    uint64_t getTotal() const;

    const Options options;

private:
    uint8_t splitTile(std::vector<ProjectedFeature> features_,
                      uint8_t z_,
                      uint32_t x_,
                      uint32_t y_,
                      uint8_t cz = 0,
                      uint32_t cx = 0,
                      uint32_t cy = 0);

private:
    std::map<uint64_t, Tile> tiles;
    std::map<uint8_t, uint16_t> stats;
    uint64_t total = 0;
};

} // namespace geojsonvt
} // namespace mapbox

#endif // MAPBOX_GEOJSONVT
