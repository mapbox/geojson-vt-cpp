#pragma once

#include <mapbox/geojsonvt/simplify.hpp>
#include <mapbox/geojsonvt/types.hpp>
#include <mapbox/geometry.hpp>

#include <cmath>

namespace mapbox {
namespace geojsonvt {
namespace detail {

struct project {
    const double tolerance;
    using result_type = vt_geometry;

    vt_point operator()(const geometry::point<double>& p) {
        const double sine = std::sin(p.y * M_PI / 180);
        const double x = p.x / 360 + 0.5;
        const double y =
            std::max(std::min(0.5 - 0.25 * std::log((1 + sine) / (1 - sine)) / M_PI, 1.0), 0.0);
        return { x, y, 0.0 };
    }

    vt_line_string operator()(const geometry::line_string<double>& points) {
        vt_line_string result;
        const size_t len = points.size();

        if (len == 0)
            return result;

        result.reserve(len);

        for (const auto& p : points) {
            result.push_back(operator()(p));
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

    vt_linear_ring operator()(const geometry::linear_ring<double>& ring) {
        vt_linear_ring result;
        const size_t len = ring.size();

        if (len == 0)
            return result;

        result.reserve(len);

        for (const auto& p : ring) {
            result.push_back(operator()(p));
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

    vt_polygon operator()(const geometry::polygon<double>& rings) {
        vt_polygon result;
        result.reserve(rings.size());
        for (const auto& ring : rings) {
            result.push_back(operator()(ring));
        }
        return result;
    }

    vt_multi_point operator()(const geometry::multi_point<double>& points) {
        vt_multi_point result;
        result.reserve(points.size());
        for (const auto& p : points) {
            result.push_back(operator()(p));
        }
        return result;
    }

    vt_multi_line_string operator()(const geometry::multi_line_string<double>& lines) {
        vt_multi_line_string result;
        result.reserve(lines.size());
        for (const auto& line : lines) {
            result.push_back(operator()(line));
        }
        return result;
    }

    vt_multi_polygon operator()(const geometry::multi_polygon<double>& polygons) {
        vt_multi_polygon result;
        result.reserve(polygons.size());
        for (const auto& polygon : polygons) {
            result.push_back(operator()(polygon));
        }
        return result;
    }

    vt_geometry_collection operator()(const geometry::geometry_collection<double>& geometries) {
        vt_geometry_collection result;
        result.reserve(geometries.size());
        for (const auto& geometry : geometries) {
            result.push_back(geometry::geometry<double>::visit(geometry, project { tolerance }));
        }
        return result;
    }
};

inline vt_features convert(const geometry::feature_collection<double>& features,
                           const double tolerance) {
    vt_features projected;
    projected.reserve(features.size());
    for (const auto& feature : features) {
        projected.emplace_back(geometry::geometry<double>::visit(feature.geometry, project { tolerance }), feature.properties);
    }
    return projected;
}

} // namespace detail
} // namespace geojsonvt
} // namespace mapbox
