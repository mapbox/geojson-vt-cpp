#ifndef MAPBOX_UTIL_GEOJSONVT_TEST_UTIL
#define MAPBOX_UTIL_GEOJSONVT_TEST_UTIL

#include <mapbox/geojsonvt/geojsonvt_types.hpp>

namespace mapbox {
namespace util {
namespace geojsonvt {

::std::ostream& operator<<(::std::ostream& os, ProjectedFeatureType t);
::std::ostream& operator<<(::std::ostream& os, const ProjectedPoint& p);
::std::ostream& operator<<(::std::ostream& os, const ProjectedGeometryContainer& c);

bool operator==(const ProjectedFeature& a, const ProjectedFeature& b);

bool operator==(const ProjectedGeometryContainer& a, const ProjectedGeometryContainer& b);

} // namespace geojsonvt
} // namespace util
} // namespace mapbox

#endif // MAPBOX_UTIL_GEOJSONVT_TEST_UTIL
