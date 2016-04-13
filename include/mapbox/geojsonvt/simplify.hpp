#ifndef MAPBOX_GEOJSONVT_SIMPLIFY
#define MAPBOX_GEOJSONVT_SIMPLIFY

#include "types.hpp"

namespace mapbox {
namespace geojsonvt {

class __attribute__((visibility("default"))) Simplify {
private:
    // This class has only static functions; disallow creating instances of it.
    Simplify() = delete;

public:
    static void simplify(ProjectedPoints& points, double tolerance);

private:
    static double
    getSqSegDist(const ProjectedPoint& p, const ProjectedPoint& a, const ProjectedPoint& b);
};

} // namespace geojsonvt
} // namespace mapbox

#endif // MAPBOX_GEOJSONVT_SIMPLIFY
