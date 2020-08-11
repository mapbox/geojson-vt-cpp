#pragma once

#include <mapbox/geometry.hpp>
#include <mapbox/feature.hpp>
#include <mapbox/variant.hpp>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace mapbox {
namespace geojsonvt {
namespace detail {

using vt_empty = mapbox::geometry::empty;

struct vt_point : mapbox::geometry::point<double> {
    double z = 0.0; // simplification tolerance

    vt_point(double x_, double y_, double z_) : mapbox::geometry::point<double>(x_, y_), z(z_) {
    }

    vt_point(double x_, double y_) : vt_point(x_, y_, 0.0) {
    }
};

template <uint8_t I, typename T>
inline double get(const T&);

template <>
inline double get<0>(const vt_point& p) {
    return p.x;
}
template <>
inline double get<1>(const vt_point& p) {
    return p.y;
}
template <>
inline double get<0>(const mapbox::geometry::point<double>& p) {
    return p.x;
}
template <>
inline double get<1>(const mapbox::geometry::point<double>& p) {
    return p.y;
}

template <uint8_t I>
inline double calc_progress(const vt_point&, const vt_point&, const double);

template <>
inline double calc_progress<0>(const vt_point& a, const vt_point& b, const double x) {
    return (x - a.x) / (b.x - a.x);
}

template <>
inline double calc_progress<1>(const vt_point& a, const vt_point& b, const double y) {
    return (y - a.y) / (b.y - a.y);
}

template <uint8_t I>
inline vt_point intersect(const vt_point&, const vt_point&, const double, const double);

template <>
inline vt_point intersect<0>(const vt_point& a, const vt_point& b, const double x, const double t) {
    const double y = (b.y - a.y) * t + a.y;
    return { x, y, 1.0 };
}
template <>
inline vt_point intersect<1>(const vt_point& a, const vt_point& b, const double y, const double t) {
    const double x = (b.x - a.x) * t + a.x;
    return { x, y, 1.0 };
}

using vt_multi_point = std::vector<vt_point>;

struct vt_line_string : std::vector<vt_point> {
    using container_type = std::vector<vt_point>;
    vt_line_string() = default;
    vt_line_string(std::initializer_list<vt_point> args)
      : container_type(std::move(args)) {}

    double dist = 0.0; // line length
    double segStart = 0.0;
    double segEnd = 0.0; // segStart and segEnd are distance along a line in tile units, when lineMetrics = true
};

struct vt_linear_ring : std::vector<vt_point> {
    using container_type = std::vector<vt_point>;
    vt_linear_ring() = default;
    vt_linear_ring(std::initializer_list<vt_point> args)
      : container_type(std::move(args)) {}

    double area = 0.0; // polygon ring area
};

using vt_multi_line_string = std::vector<vt_line_string>;
using vt_polygon = std::vector<vt_linear_ring>;
using vt_multi_polygon = std::vector<vt_polygon>;

struct vt_geometry_collection;

using vt_geometry = mapbox::util::variant<vt_empty,
                                          vt_point,
                                          vt_line_string,
                                          vt_polygon,
                                          vt_multi_point,
                                          vt_multi_line_string,
                                          vt_multi_polygon,
                                          vt_geometry_collection>;

struct vt_geometry_collection : std::vector<vt_geometry> {};

using null_value = mapbox::feature::null_value_t;
using property_map = mapbox::feature::property_map;
using identifier = mapbox::feature::identifier;
using value = mapbox::feature::value;

template <class T>
struct vt_geometry_type;

template <>
struct vt_geometry_type<geometry::empty> {
    using type = vt_empty;
};
template <>
struct vt_geometry_type<geometry::point<double>> {
    using type = vt_point;
};
template <>
struct vt_geometry_type<geometry::line_string<double>> {
    using type = vt_line_string;
};
template <>
struct vt_geometry_type<geometry::polygon<double>> {
    using type = vt_polygon;
};
template <>
struct vt_geometry_type<geometry::multi_point<double>> {
    using type = vt_multi_point;
};
template <>
struct vt_geometry_type<geometry::multi_line_string<double>> {
    using type = vt_multi_line_string;
};
template <>
struct vt_geometry_type<geometry::multi_polygon<double>> {
    using type = vt_multi_polygon;
};
template <>
struct vt_geometry_type<geometry::geometry<double>> {
    using type = vt_geometry;
};
template <>
struct vt_geometry_type<geometry::geometry_collection<double>> {
    using type = vt_geometry_collection;
};

struct vt_feature {
    vt_geometry geometry;
    std::shared_ptr<const property_map> properties;
    identifier id;

    mapbox::geometry::box<double> bbox = { { 2, 1 }, { -1, 0 } };
    uint32_t num_points = 0;

    vt_feature(const vt_geometry& geom, std::shared_ptr<const property_map> props, const identifier& id_)
        : geometry(geom), properties(std::move(props)), id(id_) {
        assert(properties);
        processGeometry();
    }

    vt_feature(const vt_geometry& geom, const property_map& props, const identifier& id_)
        : geometry(geom), properties(std::make_shared<property_map>(props)), id(id_) {
        processGeometry();
    }

private:
    void processGeometry() {
        mapbox::geometry::for_each_point(geometry, [&](const vt_point& p) {
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
