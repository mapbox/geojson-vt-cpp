#pragma once

#include <mapbox/geojsonvt/clip.hpp>
#include <mapbox/geojsonvt/types.hpp>

namespace mapbox {
namespace geojsonvt {

template <typename IntersectCallback>
vt_features wrap(const vt_features& features, double buffer, IntersectCallback intersectX) {
    // left world copy
    auto left = clip(features, -1 - buffer, buffer, 0, intersectX, -1, 2);
    // right world copy
    auto right = clip(features, 1 - buffer, 2 + buffer, 0, intersectX, -1, 2);

    if (left.empty() && right.empty())
        return features;

    // center world copy
    auto merged = clip(features, -buffer, 1 + buffer, 0, intersectX, -1, 2);

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

void shiftCoords(vt_features& features, double offset) {
    for (auto& feature : features) {
        vt_for_each_point(feature.geometry, [offset](vt_point& point) { point += offset; });
        feature.bbox.min.x += offset;
        feature.bbox.max.x += offset;
    }
}

} // namespace geojsonvt
} // namespace mapbox
