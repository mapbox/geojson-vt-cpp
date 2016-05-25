#pragma once

#include <mapbox/geojsonvt/types.hpp>

namespace mapbox {
namespace geojsonvt {

class Tile {
public:
    vt_features source_features;
    tile_features features;

    const uint8_t z;
    const uint32_t x;
    const uint32_t y;
    const uint16_t extent;
    const double tolerance;
    bool is_solid = false;

    uint32_t num_points = 0;
    uint32_t num_simplified = 0;
    mapbox::geometry::box<double> bbox = { { 2, 1 }, { -1, 0 } };

    Tile(const vt_features& source,
         const uint8_t z_,
         const uint32_t x_,
         const uint32_t y_,
         const uint16_t extent_,
         const uint16_t buffer,
         const double tolerance_)
        : z(z_),
          x(x_),
          y(y_),
          extent(extent_),
          tolerance(tolerance_),
          z2(std::pow(2, z_)),
          sq_tolerance(tolerance_ * tolerance_) {

        for (const auto& feature : source) {
            const auto& geom = feature.geometry;
            const auto& props = feature.properties;

            num_points += feature.num_points;

            if (geom.is<vt_point>()) {
                features.push_back({ { transform(geom.get<vt_point>()) }, props });

            } else if (geom.is<vt_multi_point>()) {
                features.push_back({ { transform(geom.get<vt_multi_point>()) }, props });

            } else if (geom.is<vt_line_string>()) {
                const auto line = transform(geom.get<vt_line_string>());
                if (!line.empty())
                    features.push_back({ { line }, props });

            } else if (geom.is<vt_multi_line_string>()) {
                const auto lines = transform(geom.get<vt_multi_line_string>());
                const auto size = lines.size();
                if (size == 1)
                    features.push_back({ { lines[0] }, props });
                else if (size > 1)
                    features.push_back({ { lines }, props });

            } else if (geom.is<vt_polygon>()) {
                const auto polygon = transform(geom.get<vt_polygon>());
                if (!polygon.empty())
                    features.push_back({ { polygon }, props });

            } else if (geom.is<vt_multi_polygon>()) {
                const auto polygons = transform(geom.get<vt_multi_polygon>());
                const auto size = polygons.size();
                if (size == 1)
                    features.push_back({ { polygons[0] }, props });
                else if (size > 1)
                    features.push_back({ { polygons }, props });

            } else {
                throw std::runtime_error("Geometry type not supported");
            }

            bbox.min.x = std::min(feature.bbox.min.x, bbox.min.x);
            bbox.min.y = std::min(feature.bbox.min.y, bbox.min.y);
            bbox.max.x = std::max(feature.bbox.max.x, bbox.max.x);
            bbox.max.y = std::max(feature.bbox.max.y, bbox.max.y);
        }

        is_solid = isSolid(buffer);
    }

private:
    const double z2;
    const double sq_tolerance;

    bool isSolid(const uint16_t buffer) {
        if (features.size() != 1)
            return false;

        const auto& geom = features.front().geometry;
        if (!geom.is<tile_polygon>())
            return false;

        const auto& rings = geom.get<tile_polygon>();
        if (rings.size() > 1)
            return false;

        const auto& ring = rings.front();
        if (ring.size() != 5)
            return false;

        const int16_t min = -static_cast<int16_t>(buffer);
        const int16_t max = static_cast<int16_t>(extent + buffer);
        for (const auto& p : ring) {
            if ((p.x != min && p.x != max) || (p.y != min && p.y != max))
                return false;
        }

        return true;
    }

    tile_point transform(const vt_point& p) {
        ++num_simplified;
        return { static_cast<int16_t>(std::round((p.x * z2 - x) * extent)),
                 static_cast<int16_t>(std::round((p.y * z2 - y) * extent)) };
    }

    tile_multi_point transform(const vt_multi_point& points) {
        tile_multi_point result;
        result.reserve(points.size());
        for (const auto& p : points) {
            result.push_back(transform(p));
        }
        return result;
    }

    tile_line_string transform(const vt_line_string& line) {
        tile_line_string result;
        if (line.dist > tolerance) {
            for (const auto& p : line) {
                if (p.z > sq_tolerance)
                    result.push_back(transform(p));
            }
        }
        return result;
    }

    tile_linear_ring transform(const vt_linear_ring& ring) {
        tile_linear_ring result;
        if (ring.area > sq_tolerance) {
            for (const auto& p : ring) {
                if (p.z > sq_tolerance)
                    result.push_back(transform(p));
            }
        }
        return result;
    }

    tile_multi_line_string transform(const vt_multi_line_string& lines) {
        tile_multi_line_string result;
        for (const auto& line : lines) {
            if (line.dist > tolerance)
                result.push_back(transform(line));
        }
        return result;
    }

    tile_polygon transform(const vt_polygon& rings) {
        tile_polygon result;
        for (const auto& ring : rings) {
            if (ring.area > sq_tolerance)
                result.push_back(transform(ring));
        }
        return result;
    }

    tile_multi_polygon transform(const vt_multi_polygon& polygons) {
        tile_multi_polygon result;
        for (const auto& polygon : polygons) {
            const auto p = transform(polygon);
            if (!p.empty())
                result.push_back(p);
        }
        return result;
    }
};

} // namespace geojsonvt
} // namespace mapbox
