#pragma once

#include <mapbox/geometry.hpp>
#include <mapbox/variant.hpp>

#include <vector>

namespace mapbox {
namespace geojsonvt {
namespace detail {

struct vt_point : mapbox::geometry::point<double> {
    double z = 0.0; // simplification tolerance

    vt_point(double x_, double y_, double z_) : mapbox::geometry::point<double>(x_, y_), z(z_) {
    }
};

using vt_multi_point = std::vector<vt_point>;

struct vt_line_string : std::vector<vt_point> {
    double dist = 0.0; // line length
};

struct vt_linear_ring : std::vector<vt_point> {
    double area = 0.0; // polygon ring area
};

using vt_multi_line_string = std::vector<vt_line_string>;
using vt_polygon = std::vector<vt_linear_ring>;
using vt_multi_polygon = std::vector<vt_polygon>;

using vt_geometry = mapbox::util::variant<vt_point,
                                          vt_line_string,
                                          vt_polygon,
                                          vt_multi_point,
                                          vt_multi_line_string,
                                          vt_multi_polygon>;

using property_map = std::unordered_map<std::string, mapbox::geometry::value>;

struct vt_feature {
    vt_geometry geometry;
    property_map properties;
    mapbox::geometry::box<double> bbox = { { 2, 1 }, { -1, 0 } };
    uint32_t num_points = 0;

    vt_feature(const vt_geometry& geom, const property_map& props)
        : geometry(geom), properties(props) {

        mapbox::geometry::for_each_point(geom, [&](const vt_point& p) {
            bbox.min.x = std::min(p.x, bbox.min.x);
            bbox.min.y = std::min(p.y, bbox.min.y);
            bbox.max.x = std::max(p.x, bbox.max.x);
            bbox.max.y = std::max(p.y, bbox.max.y);
            ++num_points;
        });
    }
};

using vt_features = std::vector<vt_feature>;

} // namespace detail
} // namespace geojsonvt
} // namespace mapbox
