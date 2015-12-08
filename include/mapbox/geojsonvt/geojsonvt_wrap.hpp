#ifndef MAPBOX_UTIL_GEOJSONVT_WRAP
#define MAPBOX_UTIL_GEOJSONVT_WRAP

#include "geojsonvt_types.hpp"
#include "geojsonvt_clip.hpp"

namespace mapbox {
namespace util {
namespace geojsonvt {

class __attribute__((visibility("default"))) Wrap {
public:
    static std::vector<ProjectedFeature> wrap(std::vector<ProjectedFeature>& features,
                                              double buffer,
                                              Clip::IntersectCallback intersectX);

private:
    static std::vector<ProjectedFeature>
    shiftFeatureCoords(const std::vector<ProjectedFeature>& features, int8_t offset);
    static ProjectedPoints shiftCoords(const ProjectedPoints& points, int8_t offset);
};

} // namespace geojsonvt
} // namespace util
} // namespace mapbox

#endif // MAPBOX_UTIL_GEOJSONVT_WRAP
