#ifndef MAPBOX_UTIL_GEOJSONVT_CLIP
#define MAPBOX_UTIL_GEOJSONVT_CLIP

#include "geojsonvt_types.hpp"

#include <vector>

namespace mapbox {
namespace util {
namespace geojsonvt {

class Clip {
private:
    // This class has only static functions; disallow creating instances of it.
    Clip() = delete;

public:
    typedef ProjectedPoint (*IntersectCallback)(const ProjectedPoint&,
                                                const ProjectedPoint&,
                                                double);

    static std::vector<ProjectedFeature> clip(const std::vector<ProjectedFeature>& features,
                                              uint32_t scale,
                                              double k1,
                                              double k2,
                                              uint8_t axis,
                                              IntersectCallback intersect,
                                              double minAll,
                                              double maxAll);

private:
    static ProjectedGeometryContainer
    clipPoints(const ProjectedGeometryContainer& geometry, double k1, double k2, uint8_t axis);

    static ProjectedGeometryContainer clipGeometry(const ProjectedGeometryContainer& geometry,
                                                   double k1,
                                                   double k2,
                                                   uint8_t axis,
                                                   IntersectCallback intersect,
                                                   bool closed);

    static ProjectedGeometryContainer newSlice(ProjectedGeometryContainer& slices,
                                               ProjectedGeometryContainer& slice,
                                               double area,
                                               double dist);
};

} // namespace geojsonvt
} // namespace util
} // namespace mapbox

#endif // MAPBOX_UTIL_GEOJSONVT_CLIP
