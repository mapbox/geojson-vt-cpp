#include <mapbox/geojsonvt/geojsonvt_wrap.hpp>

namespace mapbox {
namespace util {
namespace geojsonvt {

std::vector<ProjectedFeature> Wrap::wrap(std::vector<ProjectedFeature>& features,
                                         double buffer,
                                         Clip::IntersectCallback intersectX) {
    // left world copy
    const auto left = Clip::clip(features, 1, -1 - buffer, buffer, 0, intersectX, -1, 2);
    // right world copy
    const auto right = Clip::clip(features, 1, 1 - buffer, 2 + buffer, 0, intersectX, -1, 2);

    if (left.size() || right.size()) {
        // center world copy
        auto merged = Clip::clip(features, 1, -buffer, 1 + buffer, 0, intersectX, -1, 2);

        if (left.size()) {
            // merge left into center
            const auto shifted = shiftFeatureCoords(left, 1);
            merged.insert(merged.begin(), shifted.begin(), shifted.end());
        }
        if (right.size()) {
            // merge right into center
            const auto shifted = shiftFeatureCoords(right, -1);
            merged.insert(merged.end(), shifted.begin(), shifted.end());
        }
        return merged;
    } else {
        return features;
    }
}

std::vector<ProjectedFeature>
Wrap::shiftFeatureCoords(const std::vector<ProjectedFeature>& features, int8_t offset) {
    std::vector<ProjectedFeature> newFeatures;

    for (auto& feature : features) {
        const auto type = feature.type;

        ProjectedGeometry newGeometry;

        if (type == ProjectedFeatureType::Point) {
            newGeometry = shiftCoords(feature.geometry.get<ProjectedGeometryContainer>(), offset);
        } else {
            newGeometry.set<ProjectedGeometryContainer>();

            for (auto& member : feature.geometry.get<ProjectedGeometryContainer>().members) {
                auto& geometry = member.get<ProjectedGeometryContainer>();
                newGeometry.get<ProjectedGeometryContainer>().members.push_back(
                    shiftCoords(geometry, offset));
            }
        }

        newFeatures.emplace_back(std::move(newGeometry), type, feature.tags,
                                 ProjectedPoint{ feature.min.x + offset, feature.min.y },
                                 ProjectedPoint{ feature.max.x + offset, feature.max.x });
    }

    return newFeatures;
}

ProjectedGeometryContainer Wrap::shiftCoords(const ProjectedGeometryContainer& points,
                                             int8_t offset) {
    ProjectedGeometryContainer newPoints;
    newPoints.area = points.area;
    newPoints.dist = points.dist;

    for (auto& member : points.members) {
        auto& point = member.get<ProjectedPoint>();
        newPoints.members.emplace_back(ProjectedPoint{ point.x + offset, point.y, point.z });
    }
    return newPoints;
}

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
