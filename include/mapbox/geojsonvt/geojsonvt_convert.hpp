#ifndef MAPBOX_UTIL_GEOJSONVT_CONVERT
#define MAPBOX_UTIL_GEOJSONVT_CONVERT

#include "geojsonvt_types.hpp"

#include <rapidjson/document.h>

#include <vector>

namespace mapbox {
namespace util {
namespace geojsonvt {

using JSDocument = rapidjson::Document;
using JSValue = rapidjson::Value;

class __attribute__ ((visibility ("default"))) Convert {
private:
    // This class has only static functions; disallow creating instances of it.
    Convert() = delete;

public:
    static std::vector<ProjectedFeature> convert(const JSDocument& data, double tolerance);

    static ProjectedFeature
    create(Tags tags, ProjectedFeatureType type, ProjectedGeometry geometry);

    static ProjectedRing projectRing(const std::vector<LonLat>& lonlats,
                                              double tolerance = 0);

private:
    static void convertFeature(std::vector<ProjectedFeature>& features,
                               const JSValue& feature,
                               double tolerance);

    static void convertGeometry(std::vector<ProjectedFeature>& features,
                                const Tags& tags,
                                const JSValue& geometry,
                                double tolerance);

    static ProjectedPoint projectPoint(const LonLat& p);

    static void calcSize(ProjectedRing& ring);

    static void calcBBox(ProjectedFeature& feature);

    static void calcRingBBox(ProjectedPoint& minPoint,
                             ProjectedPoint& maxPoint,
                             const ProjectedPoints& points);
};

} // namespace geojsonvt
} // namespace util
} // namespace mapbox

#endif // MAPBOX_UTIL_GEOJSONVT_CONVERT
