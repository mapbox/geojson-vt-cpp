#ifndef MAPBOX_GEOJSONVT_TYPES
#define MAPBOX_GEOJSONVT_TYPES

#include <mapbox/variant.hpp>

#include <array>
#include <map>
#include <string>
#include <vector>

namespace mapbox {
namespace geojsonvt {

struct __attribute__((visibility("default"))) LonLat {
    LonLat(std::array<double, 2> const& coordinates) : lon(coordinates[0]), lat(coordinates[1]) {
    }

    LonLat(double lon_, double lat_) : lon(lon_), lat(lat_) {
    }

    double lon;
    double lat;
};

#pragma mark -

class __attribute__((visibility("default"))) ProjectedPoint {
public:
    inline ProjectedPoint(double x_ = -1, double y_ = -1, double z_ = -1) : x(x_), y(y_), z(z_) {
    }

    inline bool isValid() const {
        return (x >= 0 && y >= 0 && z >= 0);
    }

    inline bool operator==(const ProjectedPoint& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    inline bool operator!=(const ProjectedPoint& rhs) const {
        return !operator==(rhs);
    }

public:
    double x = -1;
    double y = -1;
    double z = -1;
};

#pragma mark -

using ProjectedPoints = std::vector<ProjectedPoint>;

class __attribute__((visibility("default"))) ProjectedRing {
public:
    ProjectedRing() {
    }
    ProjectedRing(ProjectedPoints const& points_) : points(points_) {
    }

public:
    ProjectedPoints points;
    double area = 0;
    double dist = 0;
};

#pragma mark -

using ProjectedRings = std::vector<ProjectedRing>;
using ProjectedGeometry = mapbox::util::variant<ProjectedPoints, ProjectedRings>;

#pragma mark -

using Tags = std::map<std::string, std::string>;

#pragma mark -

enum class ProjectedFeatureType : uint8_t { Point = 1, LineString = 2, Polygon = 3 };

#pragma mark -

class __attribute__((visibility("default"))) ProjectedFeature {
public:
    ProjectedFeature(ProjectedGeometry const& geometry_,
                     ProjectedFeatureType type_,
                     Tags const& tags_,
                     ProjectedPoint const& min_ = { 2, 1 },  // initial bbox values;
                     ProjectedPoint const& max_ = { -1, 0 }) // coords are usually in [0..1] range
        : geometry(geometry_),
          type(type_),
          tags(tags_),
          min(min_),
          max(max_) {
    }

public:
    ProjectedGeometry geometry;
    ProjectedFeatureType type;
    Tags tags;
    ProjectedPoint min;
    ProjectedPoint max;
};

#pragma mark -

class __attribute__((visibility("default"))) TilePoint {
public:
    TilePoint(int16_t x_, int16_t y_) : x(x_), y(y_) {
    }

public:
    const int16_t x = 0;
    const int16_t y = 0;
};

#pragma mark -

using TilePoints = std::vector<TilePoint>;
using TileRings = std::vector<TilePoints>;
using TileGeometry = mapbox::util::variant<TilePoints, TileRings>;

#pragma mark -

typedef ProjectedFeatureType TileFeatureType;

#pragma mark -

class __attribute__((visibility("default"))) TileFeature {
public:
    TileFeature(ProjectedGeometry const& geometry_, TileFeatureType type_, Tags const& tags_)
        : geometry(geometry_), type(type_), tags(tags_) {
    }

public:
    ProjectedGeometry geometry;
    TileGeometry tileGeometry;
    TileFeatureType type;
    Tags tags;
};

} // namespace geojsonvt
} // namespace mapbox

#endif // MAPBOX_GEOJSONVT_TYPES
