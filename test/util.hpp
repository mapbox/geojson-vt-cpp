#pragma once

#include <mapbox/geojsonvt/types.hpp>

#include <sstream>

namespace mapbox {
namespace geojsonvt {
namespace detail {

std::ostream& operator<<(std::ostream& os, const vt_point& p) {
    return os << "[" << p.x << "," << p.y << "]";
}

std::ostream& operator<<(std::ostream& os, const vt_geometry& geom) {
    vt_geometry::visit(geom, [&](const auto& g) { os << g; });
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& items) {
    os << "[";
    size_t size = items.size();
    for (size_t i = 0; i < size; i++) {
        os << items[i];
        if (i < size - 1)
            os << ",";
    }
    return os << "]";
}

} // namespace detail
} // namespace geojsonvt
} // namespace mapbox
