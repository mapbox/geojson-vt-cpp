#pragma once

#include <algorithm>
#include <cmath>
#include <mapbox/geojsonvt/types.hpp>

// ToDo: move into geometry.hpp once dependent projects upgrade to geometry.hpp v2
namespace  {
template <class... Ts> void ignore(Ts&&...) {}

template <class T> void ignore(const std::initializer_list<T>&) {}

// Handle the zero-argument case.
inline void ignore(const std::initializer_list<int>&) {}

inline void combine_hashes(std::size_t& seed, const std::size_t hash) {
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class T>
void hash_combine(std::size_t& seed, const T& v) {
    combine_hashes(seed, std::hash<T>{}(v));
}

template <class... Args>
std::size_t hash(Args&&... args) {
    std::size_t seed = 0;
    ignore({ (hash_combine(seed, args), 0)... });
    return seed;
}

const std::size_t kArrayHash = hash(std::string{"Array"});
const std::size_t kNullValueHash = hash(std::numeric_limits<double>::quiet_NaN());
const std::size_t kMapHash = hash(std::string{"Map"});
const std::size_t kGeometryHash = hash(std::string{"geometry"});
const std::size_t kGeometriesHash = hash(std::string{"geometries"});
const std::size_t kCoordinatesHash = hash(std::string{"coordinates"});
const std::size_t kFeatureHash = hash(std::string{"Feature"});
const std::size_t kFeaturesHash = hash(std::string{"Features"});
const std::size_t kIdHash = hash(std::string{"id"});
const std::size_t kPropertiesHash = hash(std::string{"properties"});
const std::size_t kFeatureCollectionHash = hash(std::string{"FeatureCollection"});

struct ValueHasher {
    std::size_t operator()(mapbox::feature::null_value_t) {
        // TODO: choose a proper value to represent null value
        return kNullValueHash;
    }

    std::size_t operator()(bool t) { return hash(t); }

    std::size_t operator()(int64_t t) { return hash(t); }

    std::size_t operator()(uint64_t t) { return hash(t); }

    std::size_t operator()(double t) { return hash(t); }

    std::size_t operator()(const std::string& t) { return hash(t); }

    std::size_t operator()(const std::vector<::mapbox::feature::value>& array) {
        std::size_t seed{0};
        // Represent Array starts
        combine_hashes(seed, kArrayHash);
        for (const auto& item : array) {
            hash_combine(seed, ::mapbox::feature::value::visit(item, *this));
        }
        return seed;
    }

    std::size_t operator()(const std::unordered_map<std::string, ::mapbox::feature::value>& map) {
        std::size_t seed{0};
        // Represent map starts
        combine_hashes(seed, kMapHash);
        for (const auto& property : map) {
            hash_combine(seed, property.first);
            hash_combine(seed, ::mapbox::feature::value::visit(property.second, *this));
        }
        return seed;
    }
};

struct to_type {
public:
    const char* operator()(const mapbox::geometry::empty&) { return "Empty"; }

    const char* operator()(const mapbox::geometry::point<int16_t>&) { return "Point"; }

    const char* operator()(const mapbox::geometry::line_string<int16_t>&) { return "LineString"; }

    const char* operator()(const mapbox::geometry::polygon<int16_t>&) { return "Polygon"; }

    const char* operator()(const mapbox::geometry::multi_point<int16_t>&) { return "MultiPoint"; }

    const char* operator()(const mapbox::geometry::multi_line_string<int16_t>&) { return "MultiLineString"; }

    const char* operator()(const mapbox::geometry::multi_polygon<int16_t>&) { return "MultiPolygon"; }

    const char* operator()(const mapbox::geometry::geometry_collection<int16_t>&) { return "GeometryCollection"; }
};

struct GeometryHasher {
    // Handles line_string, polygon, multi_point, multi_line_string, multi_polygon, and geometry_collection.
    template <class E>
    std::size_t unwrap_vector(const std::vector<E>& vector) {
        std::size_t seed{0};
        // Represent Array starts
        combine_hashes(seed, kArrayHash);
        for (const auto& item : vector) {
            hash_combine(seed, operator()(item));
        }
        return seed;
    }

    std::size_t operator()(const mapbox::geometry::line_string<int16_t>& element) {
        return unwrap_vector(element);
    }

    std::size_t operator()(const mapbox::geometry::polygon<int16_t>& element) {
        return unwrap_vector(element);
    }

    std::size_t operator()(const mapbox::geometry::multi_point<int16_t>& element) {
        return unwrap_vector(element);
    }

    std::size_t operator()(const mapbox::geometry::multi_line_string<int16_t>& element) {
        return unwrap_vector(element);
    }

    std::size_t operator()(const mapbox::geometry::multi_polygon<int16_t>& element) {
        return unwrap_vector(element);
    }

    std::size_t operator()(const mapbox::geometry::geometry_collection<int16_t>& element) {
        return unwrap_vector(element);
    }

    std::size_t operator()(const mapbox::geometry::linear_ring<int16_t>& element) {
        return unwrap_vector(element);
    }

    std::size_t operator()(const mapbox::geometry::point<int16_t>& element) {
        return hash(element.x, element.y);
    }

    std::size_t operator()(const mapbox::geometry::empty&) {
        return kNullValueHash;
    }

    std::size_t operator()(const mapbox::geometry::geometry<int16_t>& element) {
        return mapbox::geometry::geometry<int16_t>::visit(element, *this);
    }
};


std::size_t getHash(const mapbox::geometry::geometry<int16_t>& element) {
    std::size_t seed{0};
    hash_combine(seed, mapbox::geometry::geometry<int16_t>::visit(element, to_type()));
    combine_hashes(seed,
                               element.is<mapbox::geometry::geometry_collection<int16_t>>() ? kGeometriesHash : kCoordinatesHash);
    hash_combine(seed, mapbox::geometry::geometry<int16_t>::visit(element, GeometryHasher{}));

    return seed;
}

std::size_t getHash(const mapbox::feature::feature<int16_t>& element) {
    std::size_t seed{0};
    combine_hashes(seed, kFeatureHash);

    if (!element.id.is<mapbox::feature::null_value_t>()) {
        combine_hashes(seed, kIdHash);
        hash_combine(seed, mapbox::feature::identifier::visit(element.id, ValueHasher{}));
    }
    combine_hashes(seed, kGeometryHash);
    hash_combine(seed, getHash(element.geometry));
    combine_hashes(seed, kPropertiesHash);
    hash_combine(seed, ValueHasher{}(element.properties));
    return seed;
}

std::size_t getHash(const mapbox::feature::feature_collection<int16_t>& collection) {
    std::size_t seed{0};
    combine_hashes(seed, kFeatureCollectionHash);

    // Represent Array starts
    combine_hashes(seed, kArrayHash);
    for (const auto& element : collection) {
        hash_combine(seed, getHash(element));
    }
    combine_hashes(seed, kFeaturesHash);

    return seed;
}

} // namespace

namespace mapbox {
namespace geojsonvt {

struct TileKey {
    uint8_t z;
    uint32_t x;
    uint32_t y;
    size_t digest;

    bool operator==(TileKey const& o) const {
        return z == o.z && x == o.x && y == o.y && digest == o.digest;
    }
};

struct TileFeatures {
    mapbox::feature::feature_collection<int16_t> features;
    TileKey key;
};

struct Tile {
    mapbox::feature::feature_collection<int16_t> features;
    uint32_t num_points = 0;
    uint32_t num_simplified = 0;
    TileKey key;
};

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

    vt_features source_features;
    mapbox::geometry::box<double> bbox = { { 2, 1 }, { -1, 0 } };

    Tile tile;

    InternalTile(const vt_features& source,
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

        tile.key.z = z;
        tile.key.x = x;
        tile.key.y = y;
        tile.features.reserve(source.size());
        for (const auto& feature : source) {
            const auto& geom = feature.geometry;
            assert(feature.properties);
            const auto& props = feature.properties;
            const auto& id = feature.id;

            tile.num_points += feature.num_points;

            vt_geometry::visit(geom, [&](const auto& g) {
                // `this->` is a workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61636
                this->addFeature(g, *props, id);
            });

            bbox.min.x = std::min(feature.bbox.min.x, bbox.min.x);
            bbox.min.y = std::min(feature.bbox.min.y, bbox.min.y);
            bbox.max.x = std::max(feature.bbox.max.x, bbox.max.x);
            bbox.max.y = std::max(feature.bbox.max.y, bbox.max.y);
        }
        tile.key.digest = getHash(tile.features);
    }

private:
    void addFeature(const vt_empty& empty, const property_map& props, const identifier& id) {
        tile.features.emplace_back(transform(empty), props, id);
    }

    void
    addFeature(const vt_point& point, const property_map& props, const identifier& id) {
        tile.features.emplace_back(transform(point), props, id);
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
                tile.features.emplace_back(std::move(new_line), std::move(newProps), id);
            } else
                tile.features.emplace_back(std::move(new_line), props, id);
        }
    }

    void addFeature(const vt_polygon& polygon,
                    const property_map& props,
                    const identifier& id) {
        const auto new_polygon = transform(polygon);
        if (!new_polygon.empty())
            tile.features.emplace_back(std::move(new_polygon), props, id);
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
            tile.features.emplace_back(std::move(new_multi[0]), props, id);
            break;
        default:
            tile.features.emplace_back(std::move(new_multi), props, id);
            break;
        }
    }

    mapbox::geometry::empty transform(const vt_empty& empty) {
        return empty;
    }

    mapbox::geometry::point<int16_t> transform(const vt_point& p) {
        ++tile.num_simplified;
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
