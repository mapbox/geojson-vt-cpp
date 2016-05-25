#pragma once

#include <mapbox/geometry.hpp>
#include <mapbox/variant.hpp>

#include <vector>

namespace mapbox {
namespace geojsonvt {

using geojson_features = mapbox::geometry::feature_collection<double>;

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

template <typename F>
void vt_for_each_point(const vt_point& point, F&& f) {
    f(point);
}
template <typename F>
void vt_for_each_point(const vt_geometry& geom, F&& f) {
    vt_geometry::visit(geom, [&](const auto& g) { vt_for_each_point(g, f); });
}
template <typename Container, typename F>
auto vt_for_each_point(const Container& container, F&& f)
    -> decltype(container.begin(), container.end(), void()) {
    for (const auto& e : container) {
        vt_for_each_point(e, f);
    }
}
template <typename F>
void vt_for_each_point(vt_point& point, F&& f) {
    f(point);
}
template <typename F>
void vt_for_each_point(vt_geometry& geom, F&& f) {
    vt_geometry::visit(geom, [&](auto& g) { vt_for_each_point(g, f); });
}
template <typename Container, typename F>
auto vt_for_each_point(Container& container, F&& f)
    -> decltype(container.begin(), container.end(), void()) {
    for (auto& e : container) {
        vt_for_each_point(e, f);
    }
}

struct vt_feature {
    vt_geometry geometry;
    property_map properties;
    mapbox::geometry::box<double> bbox = { { 2, 1 }, { -1, 0 } };
    uint32_t num_points = 0;

    vt_feature(const vt_geometry& geom, const property_map& props)
        : geometry(geom), properties(props) {

        vt_for_each_point(geom, [&](const vt_point& p) {
            bbox.min.x = std::min(p.x, bbox.min.x);
            bbox.min.y = std::min(p.y, bbox.min.y);
            bbox.max.x = std::max(p.x, bbox.max.x);
            bbox.max.y = std::max(p.y, bbox.max.y);
            ++num_points;
        });
    }
};

using vt_features = std::vector<vt_feature>;

using tile_point = mapbox::geometry::point<int16_t>;
using tile_multi_point = mapbox::geometry::multi_point<int16_t>;
using tile_line_string = mapbox::geometry::line_string<int16_t>;
using tile_multi_line_string = mapbox::geometry::multi_line_string<int16_t>;
using tile_linear_ring = mapbox::geometry::linear_ring<int16_t>;
using tile_polygon = mapbox::geometry::polygon<int16_t>;
using tile_multi_polygon = mapbox::geometry::multi_polygon<int16_t>;
using tile_geometry = mapbox::geometry::geometry<int16_t>;

using tile_feature = mapbox::geometry::feature<int16_t>;
using tile_features = std::vector<tile_feature>;

} // namespace geojsonvt
} // namespace mapbox
