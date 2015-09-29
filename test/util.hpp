#ifndef MAPBOX_UTIL_GEOJSONVT_TEST_UTIL
#define MAPBOX_UTIL_GEOJSONVT_TEST_UTIL

#include <mapbox/geojsonvt/geojsonvt_types.hpp>

#include <rapidjson/document.h>

namespace mapbox {
namespace util {
namespace geojsonvt {

std::string loadFile(const std::string& filename);

::std::ostream& operator<<(::std::ostream& os, ProjectedFeatureType t);
::std::ostream& operator<<(::std::ostream& os, const TilePoint& p);
::std::ostream& operator<<(::std::ostream& os, const TileRing& r);
::std::ostream& operator<<(::std::ostream& os, const TileFeature& f);
::std::ostream& operator<<(::std::ostream& os, const ProjectedPoint& p);
::std::ostream& operator<<(::std::ostream& os, const ProjectedFeature& f);
::std::ostream& operator<<(::std::ostream& os, const ProjectedGeometryContainer& c);

bool operator==(const TilePoint& a, const TilePoint& b);
bool operator==(const TileRing& a, const TileRing& b);
bool operator==(const TileFeature& a, const TileFeature& b);

bool operator==(const ProjectedFeature& a, const ProjectedFeature& b);
bool operator==(const ProjectedGeometryContainer& a, const ProjectedGeometryContainer& b);

std::vector<TileFeature> parseJSONTile(const rapidjson::Value& tile);
std::vector<TileFeature> parseJSONTile(const std::string& data);
std::map<std::string, std::vector<TileFeature>> parseJSONTiles(const std::string& data);

} // namespace geojsonvt
} // namespace util
} // namespace mapbox

#endif // MAPBOX_UTIL_GEOJSONVT_TEST_UTIL
