#include <mapbox/geojsonvt/clip.hpp>

namespace mapbox {
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

    if (minAll >= k1 && maxAll <= k2) { // trivial accept
        return features;
    }
    if (minAll > k2 || maxAll < k1) { // trivial reject
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
        }
        if (min > k2 || max < k1) { // trivial reject
            continue;
        }

        ProjectedGeometry slices;

        if (type == ProjectedFeatureType::Point) {
            slices = clipPoints(geometry.get<ProjectedPoints>(), k1, k2, axis);
            if (slices.get<ProjectedPoints>().empty()) {
                continue;
            }
        } else {
            slices = clipGeometry(geometry.get<ProjectedRings>(), k1, k2, axis, intersect,
                                  (type == ProjectedFeatureType::Polygon));
            if (slices.get<ProjectedRings>().empty()) {
                continue;
            }
        }

        // if a feature got clipped, it will likely get clipped on the next zoom level as well,
        // so there's no need to recalculate bboxes
        clipped.emplace_back(slices, type, feature.tags, feature.min, feature.max);
    }

    return clipped;
}

ProjectedPoints
Clip::clipPoints(const ProjectedPoints& points, double k1, double k2, uint8_t axis) {
    ProjectedPoints slice;

    for (const auto& p : points) {
        const double ak = (axis == 0 ? p.x : p.y);

        if (ak >= k1 && ak <= k2) {
            slice.push_back(p);
        }
    }

    return slice;
}

ProjectedRings Clip::clipGeometry(const ProjectedRings& rings,
                                  double k1,
                                  double k2,
                                  uint8_t axis,
                                  IntersectCallback intersect,
                                  bool closed) {
    ProjectedRings slices;

    for (auto& ring : rings) {
        double ak = 0;
        double bk = 0;
        ProjectedPoint b;
        const double area = ring.area;
        const double dist = ring.dist;
        const size_t len = ring.points.size();
        ProjectedPoint a;

        ProjectedRing slice;

        for (size_t j = 0; j < (len - 1); ++j) {
            a = (b.isValid() ? b : ring.points[j]);
            b = ring.points[j + 1];
            ak = (bk != 0.0 ? bk : (axis == 0 ? a.x : a.y));
            bk = (axis == 0 ? b.x : b.y);

            if (ak < k1) {
                if (bk > k2) { // ---|-----|-->
                    slice.points.push_back(intersect(a, b, k1));
                    slice.points.push_back(intersect(a, b, k2));
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk >= k1) { // ---|-->  |
                    slice.points.push_back(intersect(a, b, k1));
                }
            } else if (ak > k2) {
                if (bk < k1) { // <--|-----|---
                    slice.points.push_back(intersect(a, b, k2));
                    slice.points.push_back(intersect(a, b, k1));
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk <= k2) { // |  <--|---
                    slice.points.push_back(intersect(a, b, k2));
                }
            } else {
                slice.points.push_back(a);

                if (bk < k1) { // <--|---  |
                    slice.points.push_back(intersect(a, b, k1));
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                } else if (bk > k2) { // |  ---|-->
                    slice.points.push_back(intersect(a, b, k2));
                    if (!closed) {
                        slice = newSlice(slices, slice, area, dist);
                    }
                }
                // | --> |
            }
        }

        // add the last point
        a = ring.points[len - 1];
        ak = (axis == 0 ? a.x : a.y);

        if (ak >= k1 && ak <= k2) {
            slice.points.push_back(a);
        }

        // close the polygon if its endpoints are not the same after clipping
        if (closed && !slice.points.empty()) {
            const auto& first = slice.points.front();
            const auto& last = slice.points.back();
            if (first != last) {
                slice.points.push_back(first);
            }
        }

        // add the final slice
        newSlice(slices, slice, area, dist);
    }

    return slices;
}

ProjectedRing
Clip::newSlice(ProjectedRings& slices, ProjectedRing& slice, double area, double dist) {

    if (!slice.points.empty()) {
        // we don't recalculate the area/length of the unclipped geometry because the case where it
        // goes below the visibility threshold as a result of clipping is rare, so we avoid doing
        // unnecessary work
        slice.area = area;
        slice.dist = dist;
        slices.push_back(slice);
    }

    return ProjectedRing();
}

} // namespace geojsonvt
} // namespace mapbox
