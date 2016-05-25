#pragma once

#include <mapbox/geojsonvt/simplify.hpp>
#include <mapbox/geojsonvt/types.hpp>
#include <mapbox/geometry.hpp>

#include <cmath>

using namespace mapbox::geometry;

namespace mapbox {
namespace geojsonvt {

inline vt_point project(const point<double>& p, __attribute__((unused)) const double tolerance) {
    const double sine = std::sin(p.y * M_PI / 180);
    const double x = p.x / 360 + 0.5;
    const double y =
        std::max(std::min(0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI, 1.0), 0.0);
    return { x, y, 0.0 };
}

inline vt_line_string project(const line_string<double>& points, const double tolerance) {
    vt_line_string result;
    const size_t len = points.size();
    result.reserve(len);

    for (const auto& p : points) {
        result.push_back(project(p, tolerance));
    }

    for (size_t i = 0; i < len - 1; ++i) {
        const auto& a = result[i];
        const auto& b = result[i + 1];
        // use Manhattan distance instead of Euclidian to avoid expensive square root computation
        result.dist += std::abs(b.x - a.x) + std::abs(b.y - a.y);
    }

    simplify(result, tolerance);

    return result;
}

inline vt_linear_ring project(const linear_ring<double>& ring, const double tolerance) {
    vt_linear_ring result;
    const size_t len = ring.size();
    result.reserve(len);

    for (const auto& p : ring) {
        result.push_back(project(p, tolerance));
    }

    double area = 0.0;

    for (size_t i = 0; i < len - 1; ++i) {
        const auto& a = result[i];
        const auto& b = result[i + 1];
        area += a.x * b.y - b.x * a.y;
    }
    result.area = std::abs(area / 2);

    simplify(result, tolerance);

    return result;
}

inline vt_multi_point project(const multi_point<double>& points, const double tolerance) {
    vt_multi_point result;
    result.reserve(points.size());
    for (const auto& p : points) {
        result.push_back(project(p, tolerance));
    }
    return result;
}

inline vt_multi_line_string project(const multi_line_string<double>& lines,
                                    const double tolerance) {
    vt_multi_line_string result;
    result.reserve(lines.size());

    for (const auto& line : lines) {
        result.push_back(project(line, tolerance));
    }
    return result;
}

inline vt_polygon project(const polygon<double>& rings, const double tolerance) {
    vt_polygon result;
    result.reserve(rings.size());

    for (const auto& ring : rings) {
        result.push_back(project(ring, tolerance));
    }
    return result;
}

inline vt_multi_polygon project(const multi_polygon<double>& polygons, const double tolerance) {
    vt_multi_polygon result;
    result.reserve(polygons.size());

    for (const auto& polygon : polygons) {
        result.push_back(project(polygon, tolerance));
    }
    return result;
}

inline vt_geometry project(const ::geometry<double>& geom, const double tolerance) {
    return ::geometry<double>::visit(geom, [&] (const auto& g) { return vt_geometry(project(g, tolerance)); });
}

inline vt_features convert(const geojson_features& features, const double tolerance) {
    vt_features projected;
    projected.reserve(features.size());

    for (const auto& feature : features) {
        projected.emplace_back(project(feature.geometry, tolerance), feature.properties);
    }
    return projected;
}

} // namespace geojsonvt
} // namespace mapbox
