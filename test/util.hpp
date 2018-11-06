#pragma once

#include <map>
#include <mapbox/geojsonvt/tile.hpp>
#include <mapbox/geojsonvt/types.hpp>
#include <mapbox/geometry.hpp>
#include <sstream>

namespace mapbox {
namespace geojsonvt {

std::string loadFile(const std::string& filename);
mapbox::feature::feature_collection<int16_t> parseJSONTile(const std::string& data);
std::map<std::string, mapbox::feature::feature_collection<int16_t>>
parseJSONTiles(const std::string& data);
bool operator==(const mapbox::feature::feature<short>& a,
                const mapbox::feature::feature<short>& b);
bool operator==(const mapbox::feature::feature_collection<short>& a,
                const mapbox::feature::feature_collection<short>& b);
bool operator==(const std::map<std::string, mapbox::feature::feature_collection<short>>& a,
                const std::map<std::string, mapbox::feature::feature_collection<short>>& b);
bool operator==(const mapbox::geojsonvt::Tile& a, const mapbox::geojsonvt::Tile& b);

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
