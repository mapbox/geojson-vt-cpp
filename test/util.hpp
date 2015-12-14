#ifndef MAPBOX_GEOJSONVT_TEST_UTIL
#define MAPBOX_GEOJSONVT_TEST_UTIL

#include <mapbox/geojsonvt/types.hpp>

#include <rapidjson/document.h>

namespace mapbox {
namespace geojsonvt {

std::string loadFile(const std::string& filename);

::std::ostream& operator<<(::std::ostream& os, ProjectedFeatureType t);
::std::ostream& operator<<(::std::ostream& os, const TilePoint& p);
::std::ostream& operator<<(::std::ostream& os, const TilePoints& points);
::std::ostream& operator<<(::std::ostream& os, const TileRings& rings);
::std::ostream& operator<<(::std::ostream& os, const TileFeature& f);
::std::ostream& operator<<(::std::ostream& os, const ProjectedPoint& p);
::std::ostream& operator<<(::std::ostream& os, const ProjectedFeature& f);
::std::ostream& operator<<(::std::ostream& os, const ProjectedRing& c);

bool operator==(const TilePoint& a, const TilePoint& b);
bool operator==(const TileFeature& a, const TileFeature& b);

bool operator==(const ProjectedFeature& a, const ProjectedFeature& b);
bool operator==(const ProjectedRing& a, const ProjectedRing& b);

std::vector<TileFeature> parseJSONTile(const rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::CrtAllocator>& tile);
std::vector<TileFeature> parseJSONTile(const std::string& data);
std::map<std::string, std::vector<TileFeature>> parseJSONTiles(const std::string& data);

} // namespace geojsonvt
} // namespace mapbox

#endif // MAPBOX_GEOJSONVT_TEST_UTIL
