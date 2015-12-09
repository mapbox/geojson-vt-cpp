#include <mapbox/geojsonvt/wrap.hpp>

namespace mapbox {
namespace geojsonvt {

std::vector<ProjectedFeature> Wrap::wrap(std::vector<ProjectedFeature>& features,
                                         double buffer,
                                         Clip::IntersectCallback intersectX) {
    // left world copy
    const auto left = Clip::clip(features, 1, -1 - buffer, buffer, 0, intersectX, -1, 2);
    // right world copy
    const auto right = Clip::clip(features, 1, 1 - buffer, 2 + buffer, 0, intersectX, -1, 2);

    if (!left.empty() || !right.empty()) {
        // center world copy
        auto merged = Clip::clip(features, 1, -buffer, 1 + buffer, 0, intersectX, -1, 2);

        if (!left.empty()) {
            // merge left into center
            const auto shifted = shiftFeatureCoords(left, 1);
            merged.insert(merged.begin(), shifted.begin(), shifted.end());
        }
        if (!right.empty()) {
            // merge right into center
            const auto shifted = shiftFeatureCoords(right, -1);
            merged.insert(merged.end(), shifted.begin(), shifted.end());
        }
        return merged;
    }

    return features;
}

std::vector<ProjectedFeature>
Wrap::shiftFeatureCoords(const std::vector<ProjectedFeature>& features, int8_t offset) {
    std::vector<ProjectedFeature> newFeatures;

    for (auto& feature : features) {
        const auto type = feature.type;

        ProjectedGeometry newGeometry;

        if (type == ProjectedFeatureType::Point) {
            newGeometry = shiftCoords(feature.geometry.get<ProjectedPoints>(), offset);

        } else {
            newGeometry.set<ProjectedRings>();

            for (auto& ring : feature.geometry.get<ProjectedRings>()) {
                ProjectedRing newRing;
                newRing.area = ring.area;
                newRing.dist = ring.dist;
                newRing.points = shiftCoords(ring.points, offset);
                newGeometry.get<ProjectedRings>().push_back(newRing);
            }
        }

        newFeatures.emplace_back(std::move(newGeometry), type, feature.tags,
                                 ProjectedPoint{ feature.min.x + offset, feature.min.y },
                                 ProjectedPoint{ feature.max.x + offset, feature.max.x });
    }

    return newFeatures;
}

ProjectedPoints Wrap::shiftCoords(const ProjectedPoints& points, int8_t offset) {
    ProjectedPoints newPoints;

    for (auto& p : points) {
        newPoints.emplace_back(ProjectedPoint{ p.x + offset, p.y, p.z });
    }
    return newPoints;
}

} // namespace geojsonvt
} // namespace mapbox
