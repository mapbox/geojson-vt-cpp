#include <mapbox/geojsonvt/geojsonvt_clip.hpp>

namespace mapbox {
namespace util {
namespace geojsonvt {

/* clip features between two axis-parallel lines:
 *     |        |
 *  ___|___     |     /
 * /   |   \____|____/
 *     |        |
 */

std::vector<ProjectedFeature> Clip::clip(const std::vector<ProjectedFeature>& features,
                                         uint32_t scale,
                                         double k1,
                                         double k2,
                                         uint8_t axis,
                                         IntersectCallback intersect,
                                         const double minAll,
                                         const double maxAll) {
    k1 /= scale;
    k2 /= scale;

    if (minAll >= k1 && maxAll <= k2) {
        // trivial accept
        return features;
    } else if (minAll > k2 || maxAll < k1) {
        // trivial reject
        return {};
    }

    std::vector<ProjectedFeature> clipped;

    for (const auto& feature : features) {
        const auto& geometry = feature.geometry;
        const auto type = feature.type;

        const double min = (axis == 0 ? feature.min.x : feature.min.y);
        const double max = (axis == 0 ? feature.max.x : feature.max.y);

        if (min >= k1 && max <= k2) { // trivial accept
            clipped.push_back(feature);
            continue;
        } else if (min > k2 || max < k1) { // trivial reject
            continue;
        }

        ProjectedGeometryContainer slices;

        if (type == ProjectedFeatureType::Point) {
            slices = clipPoints(geometry.get<ProjectedGeometryContainer>(), k1, k2, axis);
        } else {
            slices = clipGeometry(geometry.get<ProjectedGeometryContainer>(), k1, k2, axis,
                                  intersect, (type == ProjectedFeatureType::Polygon));
        }

        if (slices.members.size()) {
            // if a feature got clipped, it will likely get clipped on the next zoom level as well,
            // so there's no need to recalculate bboxes
            clipped.emplace_back(slices, type, feature.tags, feature.min, feature.max);
        }
    }

    return std::move(clipped);
}

ProjectedGeometryContainer
Clip::clipPoints(const ProjectedGeometryContainer& geometry, double k1, double k2, uint8_t axis) {
    ProjectedGeometryContainer slice;

    for (const auto& member : geometry.members) {
        auto& a = member.get<ProjectedPoint>();
        const double ak = (axis == 0 ? a.x : a.y);

        if (ak >= k1 && ak <= k2) {
            slice.members.push_back(a);
        }
    }

    return std::move(slice);
}

ProjectedGeometryContainer Clip::clipGeometry(const ProjectedGeometryContainer& geometry,
                                              double k1,
                                              double k2,
                                              uint8_t axis,
                                              IntersectCallback intersect,
                                              bool closed) {
    ProjectedGeometryContainer slices;

    for (auto& member : geometry.members) {
        double ak = 0;
        double bk = 0;
        ProjectedPoint b;
        auto& points = member.get<ProjectedGeometryContainer>();
        const double area = points.area;
        const double dist = points.dist;
        const size_t len = points.members.size();
        ProjectedPoint a;

        ProjectedGeometryContainer slice;

        for (size_t j = 0; j < (len - 1); ++j) {
            a = (b.isValid() ? b : points.members[j].get<ProjectedPoint>());
            b = points.members[j + 1].get<ProjectedPoint>();
            ak = (bk ? bk : (axis == 0 ? a.x : a.y));
            bk = (axis == 0 ? b.x : b.y);

            if (ak < k1) {
                if (bk > k2) { // ---|-----|-->
                    slice.members.push_back(intersect(a, b, k1));
                    slice.members.push_back(intersect(a, b, k2));
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk >= k1) { // ---|-->  |
                    slice.members.push_back(intersect(a, b, k1));
                }
            } else if (ak > k2) {
                if (bk < k1) { // <--|-----|---
                    slice.members.push_back(intersect(a, b, k2));
                    slice.members.push_back(intersect(a, b, k1));
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk <= k2) { // |  <--|---
                    slice.members.push_back(intersect(a, b, k2));
                }
            } else {
                slice.members.push_back(a);

                if (bk < k1) { // <--|---  |
                    slice.members.push_back(intersect(a, b, k1));
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk > k2) { // |  ---|-->
                    slice.members.push_back(intersect(a, b, k2));
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                }
                // | --> |
            }
        }

        // add the last point
        a = points.members[len - 1].get<ProjectedPoint>();
        ak = (axis == 0 ? a.x : a.y);

        if (ak >= k1 && ak <= k2) {
            slice.members.push_back(a);
        }

        // close the polygon if its endpoints are not the same after clipping
        if (closed && slice.members.size()) {
            const auto& first = slice.members.front().get<ProjectedPoint>();
            const auto& last = slice.members.back().get<ProjectedPoint>();
            if (first != last) {
                slice.members.push_back(first);
            }
        }

        // add the final slice
        newSlice(slices, slice, area, dist);
    }

    return std::move(slices);
}

ProjectedGeometryContainer Clip::newSlice(ProjectedGeometryContainer& slices,
                                          ProjectedGeometryContainer& slice,
                                          double area,
                                          double dist) {

    if (slice.members.size()) {
        // we don't recalculate the area/length of the unclipped geometry because the case where it
        // goes below the visibility threshold as a result of clipping is rare, so we avoid doing
        // unnecessary work
        slice.area = area;
        slice.dist = dist;
        slices.members.push_back(slice);
    }

    return ProjectedGeometryContainer();
}

} // namespace geojsonvt
} // namespace util
} // namespace mapbox
