#ifndef MAPBOX_GEOJSONVT_TRANSFORM
#define MAPBOX_GEOJSONVT_TRANSFORM

#include "tile.hpp"
#include "types.hpp"

namespace mapbox {
namespace geojsonvt {

class __attribute__((visibility("default"))) Transform {
private:
    // This class has only static functions; disallow creating instances of it.
    Transform() = delete;

public:
    static TilePoint
    transformPoint(const ProjectedPoint& p, uint16_t extent, uint32_t z2, uint32_t tx, uint32_t ty);

    static const Tile& transformTile(Tile&, uint16_t extent);
};

} // namespace geojsonvt
} // namespace mapbox

#endif // MAPBOX_GEOJSONVT_TRANSFORM
