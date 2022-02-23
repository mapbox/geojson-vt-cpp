#pragma once

#include <algorithm>
#include <set>
#include <cmath>
#include <mapbox/geojsonvt/types.hpp>

namespace mapbox {
namespace geojsonvt {

struct Tile {
    mapbox::feature::feature_collection<int16_t> features;

    // The following are here for testing purposes
    uint32_t num_points = 0;
    uint32_t num_simplified = 0;
};

using TileFeatures = Tile;

namespace detail {

class InternalTile {
public:
    const uint16_t extent;
    const uint8_t z;
    const uint32_t x;
    const uint32_t y;

    const double z2;
    const double tolerance;
    const double sq_tolerance;
    const bool lineMetrics;

    feature_container<vt_feature> source_features;
    mapbox::geometry::box<double> bbox = { { 2, 1 }, { -1, 0 } };

    std::shared_ptr<const Tile> tile;
    feature_container<mapbox::feature::feature<int16_t>> features;
    uint32_t num_points = 0;
    uint32_t num_simplified = 0;

    InternalTile(const detail::vt_features& source,
                 const uint8_t z_,
                 const uint32_t x_,
                 const uint32_t y_,
                 const uint16_t extent_,
                 const double tolerance_,
                 const bool lineMetrics_)
        : extent(extent_),
          z(z_),
          x(x_),
          y(y_),
          z2(std::pow(2, z)),
          tolerance(tolerance_),
          sq_tolerance(tolerance_ * tolerance_),
          lineMetrics(lineMetrics_) {

        insertFeatures(source);
    }

    std::shared_ptr<const Tile> getTile() {
        if (tile) {
            return tile;
        }
        auto tile_ = std::make_shared<Tile>();
        features.getFeatures(tile_->features);
        tile_->num_points = num_points;
        tile_->num_simplified = num_simplified;
        tile = tile_;
        return tile;
    }

    void insertFeatures(const vt_features& features_) {
        tile.reset();
        for (const auto& feature : features_) {
            const auto& geom = feature.geometry;
            assert(feature.properties);
            const auto& props = feature.properties;
            const auto& id = feature.id;

            if (source_features.size() || !features.size()) { // Note that if these ever both go to 0, retaining will restart even if fully split
                source_features.push_back(feature);
            }

            num_points += feature.num_points;

            vt_geometry::visit(geom, [&](const auto& g) {
                // `this->` is a workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61636
                this->addFeature(g, *props, id);
            });
            extendBox(bbox, feature.bbox);
        }
    }

    void removeFeature(const mapbox::feature::identifier& id) {
        if (id.is<mapbox::feature::null_value_t>() || !hasFeature(id)) {
            return;
        }

        source_features.erase(id);
        tile.reset();
        // ToDo: figure a way to decrease num_points (dependent on vt_features)
        features.erase(id);
    }

    bool hasFeature(const mapbox::feature::identifier &id) const {
        return features.hasFeature(id);
    }

    static void extendBox(mapbox::geometry::box<double> &bbox, const mapbox::geometry::box<double>& toAdd) {
        bbox.min.x = std::min(toAdd.min.x, bbox.min.x);
        bbox.min.y = std::min(toAdd.min.y, bbox.min.y);
        bbox.max.x = std::max(toAdd.max.x, bbox.max.x);
        bbox.max.y = std::max(toAdd.max.y, bbox.max.y);
    }

    static inline uint64_t toID(uint8_t z, uint32_t x, uint32_t y) {
        return (((1ull << z) * y + x) * 32) + z;
    }

    static inline std::tuple<uint32_t, uint32_t, uint8_t> fromID(uint64_t id) {
        uint8_t z = id & 31; // 0b11111
        const uint64_t d = (1ull << uint32_t(z));
        const uint64_t xy = id >> 5;
        return std::make_tuple<uint32_t, uint32_t, uint8_t>(
                    xy % d,
                    xy / d,
                    std::move(z));
    }

private:


    void addFeature(const vt_empty& empty, const property_map& props, const identifier& id) {
        features.emplace_back(transform(empty), props, id);
    }

    void
    addFeature(const vt_point& point, const property_map& props, const identifier& id) {
        features.emplace_back(transform(point), props, id);
    }

    void addFeature(const vt_line_string& line,
                    const property_map& props,
                    const identifier& id) {
        const auto new_line = transform(line);
        if (!new_line.empty()) {
            if (lineMetrics) {
                property_map newProps = props;
                newProps.emplace(std::make_pair<std::string, value>("mapbox_clip_start", line.segStart / line.dist));
                newProps.emplace(std::make_pair<std::string, value>("mapbox_clip_end", line.segEnd / line.dist));
                features.emplace_back(std::move(new_line), std::move(newProps), id);
            } else
                features.emplace_back(std::move(new_line), props, id);
        }
    }

    void addFeature(const vt_polygon& polygon,
                    const property_map& props,
                    const identifier& id) {
        const auto new_polygon = transform(polygon);
        if (!new_polygon.empty())
            features.emplace_back(std::move(new_polygon), props, id);
    }

    void addFeature(const vt_geometry_collection& collection,
                    const property_map& props,
                    const identifier& id) {
        for (const auto& geom : collection) {
            vt_geometry::visit(geom, [&](const auto& g) {
                // `this->` is a workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61636
                this->addFeature(g, props, id);
            });
        }
    }

    template <class T>
    void addFeature(const T& multi, const property_map& props, const identifier& id) {
        const auto new_multi = transform(multi);

        switch (new_multi.size()) {
        case 0:
            break;
        case 1:
            features.emplace_back(std::move(new_multi[0]), props, id);
            break;
        default:
            features.emplace_back(std::move(new_multi), props, id);
            break;
        }
    }

    mapbox::geometry::empty transform(const vt_empty& empty) {
        return empty;
    }

    mapbox::geometry::point<int16_t> transform(const vt_point& p) {
        ++num_simplified;
        return { static_cast<int16_t>(::round((p.x * z2 - x) * extent)),
                 static_cast<int16_t>(::round((p.y * z2 - y) * extent)) };
    }

    mapbox::geometry::multi_point<int16_t> transform(const vt_multi_point& points) {
        mapbox::geometry::multi_point<int16_t> result;
        result.reserve(points.size());
        for (const auto& p : points) {
            result.emplace_back(transform(p));
        }
        return result;
    }

    mapbox::geometry::line_string<int16_t> transform(const vt_line_string& line) {
        mapbox::geometry::line_string<int16_t> result;
        if (line.dist > tolerance) {
            result.reserve(line.size());
            for (const auto& p : line) {
                if (p.z > sq_tolerance)
                    result.emplace_back(transform(p));
            }
        }
        return result;
    }

    mapbox::geometry::linear_ring<int16_t> transform(const vt_linear_ring& ring) {
        mapbox::geometry::linear_ring<int16_t> result;
        if (ring.area > sq_tolerance) {
            result.reserve(ring.size());
            for (const auto& p : ring) {
                if (p.z > sq_tolerance)
                    result.emplace_back(transform(p));
            }
        }
        return result;
    }

    mapbox::geometry::multi_line_string<int16_t> transform(const vt_multi_line_string& lines) {
        mapbox::geometry::multi_line_string<int16_t> result;
        result.reserve(lines.size());
        for (const auto& line : lines) {
            if (line.dist > tolerance)
                result.emplace_back(transform(line));
        }
        return result;
    }

    mapbox::geometry::polygon<int16_t> transform(const vt_polygon& rings) {
        mapbox::geometry::polygon<int16_t> result;
        result.reserve(rings.size());
        for (const auto& ring : rings) {
            if (ring.area > sq_tolerance)
                result.emplace_back(transform(ring));
        }
        return result;
    }

    mapbox::geometry::multi_polygon<int16_t> transform(const vt_multi_polygon& polygons) {
        mapbox::geometry::multi_polygon<int16_t> result;
        result.reserve(polygons.size());
        for (const auto& polygon : polygons) {
            const auto p = transform(polygon);
            if (!p.empty())
                result.emplace_back(std::move(p));
        }
        return result;
    }
};

} // namespace detail
} // namespace geojsonvt
} // namespace mapbox
