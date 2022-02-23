#pragma once

#include <mapbox/geojsonvt/clip.hpp>
#include <mapbox/geojsonvt/types.hpp>

namespace mapbox {
namespace geojsonvt {
namespace detail {

template <template <typename...> class Cont>
inline void shiftCoords(Cont<vt_feature>& features, double offset) {
    for (auto& feature : features) {
        mapbox::geometry::for_each_point(feature.geometry,
                                         [offset](vt_point& point) { point.x += offset; });
        feature.bbox.min.x += offset;
        feature.bbox.max.x += offset;
    }
}

template <template <typename...> class InputCont, template <typename...> class OutputCont = InputCont>
inline OutputCont<vt_feature> wrap(const InputCont<vt_feature>& features, double buffer, const bool lineMetrics) {
    // left world copy
    auto left = clip<0>(features, -1 - buffer, buffer, -1, 2, lineMetrics);
    // right world copy
    auto right = clip<0>(features, 1 - buffer, 2 + buffer, -1, 2, lineMetrics);

    if (left.empty() && right.empty())
        return features;

    // center world copy
    auto merged = clip<0>(features, -buffer, 1 + buffer, -1, 2, lineMetrics);

    if (!left.empty()) {
        // merge left into center
        shiftCoords(left, 1.0);
        merged.insert(merged.begin(), left.begin(), left.end());
    }
    if (!right.empty()) {
        // merge right into center
        shiftCoords(right, -1.0);
        merged.insert(merged.end(), right.begin(), right.end());
    }
    return merged;
}

} // namespace detail
} // namespace geojsonvt
} // namespace mapbox
