#ifndef MAPBOX_UTIL_GEOJSONVT_SIMPLIFY
#define MAPBOX_UTIL_GEOJSONVT_SIMPLIFY

#include <mapbox/geojsonvt/geojsonvt_types.hpp>

namespace mapbox {
namespace util {
namespace geojsonvt {

class Simplify {
private:
    // This class has only static functions; disallow creating instances of it.
    Simplify() = delete;

public:
    static void simplify(ProjectedGeometryContainer& points, double tolerance);

private:
    static double
    getSqSegDist(const ProjectedPoint& p, const ProjectedPoint& a, const ProjectedPoint& b);
};

} // namespace geojsonvt
} // namespace util
} // namespace mapbox

#endif // MAPBOX_UTIL_GEOJSONVT_SIMPLIFY
