#ifndef MAPBOX_GEOJSONVT_CONVERT
#define MAPBOX_GEOJSONVT_CONVERT

#include "types.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#include <rapidjson/document.h>
#pragma GCC diagnostic pop

#include <vector>

namespace mapbox {
namespace geojsonvt {

// Use the CrtAllocator, because the MemoryPoolAllocator is broken on ARM
// https://github.com/miloyip/rapidjson/issues/200
// https://github.com/miloyip/rapidjson/issues/301
// https://github.com/miloyip/rapidjson/issues/388
using JSDocument = rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::CrtAllocator>;
using JSValue = rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::CrtAllocator>;

class __attribute__((visibility("default"))) Convert {
private:
    // This class has only static functions; disallow creating instances of it.
    Convert() = delete;

public:
    static std::vector<ProjectedFeature> convert(const JSValue& data, double tolerance);

    static ProjectedFeature
    create(const Tags& tags, ProjectedFeatureType type, ProjectedGeometry const& geometry);

    static ProjectedRing projectRing(geometry::linear_ring<double> const& points,
                                     double tolerance = 0);

private:
    static void convertFeature(std::vector<ProjectedFeature>& features,
                               const JSValue& feature,
                               double tolerance);

    static void convertGeometry(std::vector<ProjectedFeature>& features,
                                const Tags& tags,
                                const JSValue& geom,
                                double tolerance);

    static ProjectedPoint projectPoint(geometry::point<double> const& pt);

    static void calcSize(ProjectedRing& ring);

    static void calcBBox(ProjectedFeature& feature);

    static void
    calcRingBBox(ProjectedPoint& minPoint, ProjectedPoint& maxPoint, const ProjectedPoints& points);

    static geometry::point<double> readCoordinate(const JSValue& value);

    static ProjectedRing readCoordinateRing(const JSValue& rawRing, double tolerance);

    static const JSValue& validArray(const JSValue& value, rapidjson::SizeType minSize = 1);
};

} // namespace geojsonvt
} // namespace mapbox

#endif // MAPBOX_GEOJSONVT_CONVERT
