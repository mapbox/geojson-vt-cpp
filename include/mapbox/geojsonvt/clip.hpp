#ifndef MAPBOX_GEOJSONVT_CLIP
#define MAPBOX_GEOJSONVT_CLIP

#include "types.hpp"

#include <vector>

namespace mapbox {
namespace geojsonvt {

class __attribute__((visibility("default"))) Clip {
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
    static ProjectedPoints
    clipPoints(const ProjectedPoints& points, double k1, double k2, uint8_t axis);
    static ProjectedRings clipGeometry(const ProjectedRings& rings,
                                       double k1,
                                       double k2,
                                       uint8_t axis,
                                       IntersectCallback intersect,
                                       bool closed);
    static ProjectedRing
    newSlice(ProjectedRings& slices, ProjectedRing& slice, double area, double dist);
};

} // namespace geojsonvt
} // namespace mapbox

#endif // MAPBOX_GEOJSONVT_CLIP
