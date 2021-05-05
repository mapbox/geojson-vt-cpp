#pragma once

#include <mapbox/geojsonvt/types.hpp>

namespace mapbox {
namespace geojsonvt {
namespace detail {

template <uint8_t I>
class clipper {
public:
    clipper(double k1_, double k2_, bool lineMetrics_ = false)
        : k1(k1_), k2(k2_), lineMetrics(lineMetrics_) {}

    const double k1;
    const double k2;
    const bool lineMetrics;

    vt_geometry operator()(const vt_empty& empty) const {
        return empty;
    }

    vt_geometry operator()(const vt_point& point) const {
        return point;
    }

    vt_geometry operator()(const vt_multi_point& points) const {
        vt_multi_point part;
        for (const auto& p : points) {
            const double ak = get<I>(p);
            if (ak >= k1 && ak <= k2)
                part.emplace_back(p);
        }
        return part;
    }

    vt_geometry operator()(const vt_line_string& line) const {
        vt_multi_line_string parts;
        clipLine(line, parts);
        if (parts.size() == 1)
            return parts[0];
        else
            return parts;
    }

    vt_geometry operator()(const vt_multi_line_string& lines) const {
        vt_multi_line_string parts;
        for (const auto& line : lines) {
            clipLine(line, parts);
        }
        if (parts.size() == 1)
            return parts[0];
        else
            return parts;
    }

    vt_geometry operator()(const vt_polygon& polygon) const {
        vt_polygon result;
        for (const auto& ring : polygon) {
            auto new_ring = clipRing(ring);
            if (!new_ring.empty())
                result.emplace_back(std::move(new_ring));
        }
        return result;
    }

    vt_geometry operator()(const vt_multi_polygon& polygons) const {
        vt_multi_polygon result;
        for (const auto& polygon : polygons) {
            vt_polygon p;
            for (const auto& ring : polygon) {
                auto new_ring = clipRing(ring);
                if (!new_ring.empty())
                    p.emplace_back(std::move(new_ring));
            }
            if (!p.empty())
                result.emplace_back(std::move(p));
        }
        return result;
    }

    vt_geometry operator()(const vt_geometry_collection& geometries) const {
        vt_geometry_collection result;
        for (const auto& geometry : geometries) {
            vt_geometry::visit(geometry,
                               [&](const auto& g) { result.emplace_back(this->operator()(g)); });
        }
        return result;
    }

private:
    vt_line_string newSlice(const vt_line_string& line) const {
        vt_line_string slice;
        slice.dist = line.dist;
        if (lineMetrics) {
            slice.segStart = line.segStart;
            slice.segEnd = line.segEnd;
        }
        return slice;
    }

    void clipLine(const vt_line_string& line, vt_multi_line_string& slices) const {
        const size_t len = line.size();
        double lineLen = line.segStart;
        double segLen = 0.0;
        double t = 0.0;

        if (len < 2)
            return;

        vt_line_string slice = newSlice(line);

        for (size_t i = 0; i < (len - 1); ++i) {
            const auto& a = line[i];
            const auto& b = line[i + 1];
            const double ak = get<I>(a);
            const double bk = get<I>(b);
            const bool isLastSeg = (i == (len - 2));

            if (lineMetrics) segLen = ::hypot((b.x - a.x), (b.y - a.y));

            if (ak < k1) {
                if (bk > k2) { // ---|-----|-->
                    t = calc_progress<I>(a, b, k1);
                    slice.emplace_back(intersect<I>(a, b, k1, t));
                    if (lineMetrics) slice.segStart = lineLen + segLen * t;

                    t = calc_progress<I>(a, b, k2);
                    slice.emplace_back(intersect<I>(a, b, k2, t));
                    if (lineMetrics) slice.segEnd = lineLen + segLen * t;
                    slices.emplace_back(std::move(slice));

                    slice = newSlice(line);

                } else if (bk > k1) { // ---|-->  |
                    t = calc_progress<I>(a, b, k1);
                    slice.emplace_back(intersect<I>(a, b, k1, t));
                    if (lineMetrics) slice.segStart = lineLen + segLen * t;
                    if (isLastSeg) slice.emplace_back(b); // last point

                } else if (bk == k1 && !isLastSeg) { // --->|..  |
                    if (lineMetrics) slice.segStart = lineLen + segLen;
                    slice.emplace_back(b);
                }
            } else if (ak > k2) {
                if (bk < k1) { // <--|-----|---
                    t = calc_progress<I>(a, b, k2);
                    slice.emplace_back(intersect<I>(a, b, k2, t));
                    if (lineMetrics) slice.segStart = lineLen + segLen * t;

                    t = calc_progress<I>(a, b, k1);
                    slice.emplace_back(intersect<I>(a, b, k1, t));
                    if (lineMetrics) slice.segEnd = lineLen + segLen * t;

                    slices.emplace_back(std::move(slice));

                    slice = newSlice(line);

                } else if (bk < k2) { // |  <--|---
                    t = calc_progress<I>(a, b, k2);
                    slice.emplace_back(intersect<I>(a, b, k2, t));
                    if (lineMetrics) slice.segStart = lineLen + segLen * t;
                    if (isLastSeg) slice.emplace_back(b); // last point

                } else if (bk == k2 && !isLastSeg) { // |  ..|<---
                    if (lineMetrics) slice.segStart = lineLen + segLen;
                    slice.emplace_back(b);
                }
            } else {
                slice.emplace_back(a);

                if (bk < k1) { // <--|---  |
                    t = calc_progress<I>(a, b, k1);
                    slice.emplace_back(intersect<I>(a, b, k1, t));
                    if (lineMetrics) slice.segEnd = lineLen + segLen * t;
                    slices.emplace_back(std::move(slice));
                    slice = newSlice(line);

                } else if (bk > k2) { // |  ---|-->
                    t = calc_progress<I>(a, b, k2);
                    slice.emplace_back(intersect<I>(a, b, k2, t));
                    if (lineMetrics) slice.segEnd = lineLen + segLen * t;
                    slices.emplace_back(std::move(slice));
                    slice = newSlice(line);

                } else if (isLastSeg) { // | --> |
                    slice.emplace_back(b);
                }
            }

            if (lineMetrics) lineLen += segLen;
        }

        if (!slice.empty()) { // add the final slice
            if (lineMetrics) slice.segEnd = lineLen;
            slices.emplace_back(std::move(slice));
        }
    }

    vt_linear_ring clipRing(const vt_linear_ring& ring) const {
        const size_t len = ring.size();
        vt_linear_ring slice;
        slice.area = ring.area;

        if (len < 2)
            return slice;

        for (size_t i = 0; i < (len - 1); ++i) {
            const auto& a = ring[i];
            const auto& b = ring[i + 1];
            const double ak = get<I>(a);
            const double bk = get<I>(b);

            if (ak < k1) {
                if (bk > k1) {
                    // ---|-->  |
                    slice.emplace_back(intersect<I>(a, b, k1, calc_progress<I>(a, b, k1)));
                    if (bk > k2)
                        // ---|-----|-->
                        slice.emplace_back(intersect<I>(a, b, k2, calc_progress<I>(a, b, k2)));
                    else if (i == len - 2)
                        slice.emplace_back(b); // last point
                }
            } else if (ak > k2) {
                if (bk < k2) { // |  <--|---
                    slice.emplace_back(intersect<I>(a, b, k2, calc_progress<I>(a, b, k2)));
                    if (bk < k1) // <--|-----|---
                        slice.emplace_back(intersect<I>(a, b, k1, calc_progress<I>(a, b, k1)));
                    else if (i == len - 2)
                        slice.emplace_back(b); // last point
                }
            } else {
                // | --> |
                slice.emplace_back(a);
                if (bk < k1)
                    // <--|---  |
                    slice.emplace_back(intersect<I>(a, b, k1, calc_progress<I>(a, b, k1)));
                else if (bk > k2)
                    // |  ---|-->
                    slice.emplace_back(intersect<I>(a, b, k2, calc_progress<I>(a, b, k2)));
            }
        }

        // close the polygon if its endpoints are not the same after clipping
        if (!slice.empty()) {
            const auto& first = slice.front();
            const auto& last = slice.back();
            if (first != last) {
                slice.emplace_back(first);
            }
        }

        return slice;
    }
};

/* clip features between two axis-parallel lines:
 *     |        |
 *  ___|___     |     /
 * /   |   \____|____/
 *     |        |
 */

template <uint8_t I>
inline vt_features clip(const vt_features& features,
                        const double k1,
                        const double k2,
                        const double minAll,
                        const double maxAll,
                        const bool lineMetrics) {

    if (minAll >= k1 && maxAll < k2) // trivial accept
        return features;

    if (maxAll < k1 || minAll >= k2) // trivial reject
        return {};

    vt_features clipped;
    clipped.reserve(features.size());

    for (const auto& feature : features) {
        const auto& geom = feature.geometry;
        assert(feature.properties);
        const auto& props = feature.properties;
        const auto& id = feature.id;

        const double min = get<I>(feature.bbox.min);
        const double max = get<I>(feature.bbox.max);

        if (min >= k1 && max < k2) { // trivial accept
            clipped.emplace_back(feature);

        } else if (max < k1 || min >= k2) { // trivial reject
            continue;

        } else {
            const auto& clippedGeom = vt_geometry::visit(geom, clipper<I>{ k1, k2, lineMetrics });

            clippedGeom.match(
                [&](const auto&) {
                    clipped.emplace_back(clippedGeom, props, id);
                },
                [&](const vt_multi_line_string& result) {
                    if (lineMetrics) {
                        for (const auto& segment : result) {
                            clipped.emplace_back(segment, props, id);
                        }
                    } else {
                        clipped.emplace_back(clippedGeom, props, id);
                    }
                }
            );
        }
    }

    return clipped;
}

} // namespace detail
} // namespace geojsonvt
} // namespace mapbox
