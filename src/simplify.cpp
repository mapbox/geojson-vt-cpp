#include <mapbox/geojsonvt/simplify.hpp>

#include <stack>

namespace mapbox {
namespace geojsonvt {

// calculate simplification data using optimized Douglas-Peucker algorithm

void Simplify::simplify(ProjectedPoints& points, double tolerance) {
    const double sqTolerance = tolerance * tolerance;
    const size_t len = points.size();
    size_t first = 0;
    size_t last = len - 1;
    std::stack<size_t> stack;
    double maxSqDist = 0;
    double sqDist = 0;
    size_t index = 0;

    // always retain the endpoints (1 is the max value)
    points[first].z = 1;
    points[last].z = 1;

    // avoid recursion by using a stack
    while (last != 0u) {

        maxSqDist = 0;

        for (size_t i = (first + 1); i < last; ++i) {
            sqDist = getSqSegDist(points[i], points[first], points[last]);

            if (sqDist > maxSqDist) {
                index = i;
                maxSqDist = sqDist;
            }
        }

        if (maxSqDist > sqTolerance) {
            // save the point importance in squared pixels as a z coordinate
            points[index].z = maxSqDist;
            stack.push(first);
            stack.push(index);
            first = index;

        } else {
            if (!stack.empty()) {
                last = stack.top();
                stack.pop();
                first = stack.top();
                stack.pop();
            } else {
                last = 0;
                first = 0;
            }
        }
    }
}

// square distance from a point to a segment
double
Simplify::getSqSegDist(const ProjectedPoint& p, const ProjectedPoint& a, const ProjectedPoint& b) {
    double x = a.x;
    double y = a.y;
    double dx = b.x - a.x;
    double dy = b.y - a.y;

    if ((dx != 0.0) || (dy != 0.0)) {

        const double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / (dx * dx + dy * dy);

        if (t > 1) {
            x = b.x;
            y = b.y;

        } else if (t > 0) {
            x += dx * t;
            y += dy * t;
        }
    }

    dx = p.x - x;
    dy = p.y - y;

    return dx * dx + dy * dy;
}

} // namespace geojsonvt
} // namespace mapbox
