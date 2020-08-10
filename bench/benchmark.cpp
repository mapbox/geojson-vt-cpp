#include <benchmark/benchmark.h>
#include <mapbox/geojson.hpp>
#include <mapbox/geojson_impl.hpp>
#include <mapbox/geojsonvt.hpp>

#include "util.hpp"

static void ParseGeoJSON(::benchmark::State& state) {
    const std::string json = loadFile("data/countries.geojson");
    for (auto _ : state) {
        mapbox::geojson::parse(json).get<mapbox::geojson::feature_collection>();
    }
}
BENCHMARK(ParseGeoJSON)->Unit(benchmark::kMicrosecond);

static void GenerateTileIndex(::benchmark::State& state) {
    const std::string json = loadFile("data/countries.geojson");
    const auto features = mapbox::geojson::parse(json).get<mapbox::geojson::feature_collection>();
    mapbox::geojsonvt::Options options;
    options.indexMaxZoom = 7;
    options.indexMaxPoints = 200;

    for (auto _ : state) {
        mapbox::geojsonvt::GeoJSONVT index{ features, options };
        (void)index;
    }
}
BENCHMARK(GenerateTileIndex)->Unit(benchmark::kMicrosecond);

static void TraverseTilePyramid(::benchmark::State& state) {
    const std::string json = loadFile("data/countries.geojson");
    const auto features = mapbox::geojson::parse(json).get<mapbox::geojson::feature_collection>();
    mapbox::geojsonvt::Options options;
    options.indexMaxZoom = 7;
    options.indexMaxPoints = 200;
    mapbox::geojsonvt::GeoJSONVT index{ features, options };

    for (auto _ : state) {
        const unsigned max_z = 11;
        for (unsigned z = 0; z < max_z; ++z) {
            unsigned num_tiles = std::pow(2, z);
            for (unsigned x = 0; x < num_tiles; ++x) {
                for (unsigned y = 0; y < num_tiles; ++y) {
                    index.getTile(z, x, y);
                }
            }
        }
    }
}
BENCHMARK(TraverseTilePyramid)->Unit(benchmark::kMillisecond)->Iterations(3);

static void LargeGeoJSONParse(::benchmark::State& state) {
    const std::string json = loadFile("test/fixtures/points.geojson");
    for (auto _ : state) {
        mapbox::geojson::parse(json).get<mapbox::geojson::feature_collection>();
    }
}
BENCHMARK(LargeGeoJSONParse)->Unit(benchmark::kMicrosecond);

static void LargeGeoJSONTileIndex(::benchmark::State& state) {
    const std::string json = loadFile("test/fixtures/points.geojson");
    const auto features = mapbox::geojson::parse(json).get<mapbox::geojson::feature_collection>();
    mapbox::geojsonvt::Options options;
    for (auto _ : state) {
        mapbox::geojsonvt::GeoJSONVT index{ features, options };
    }
}
BENCHMARK(LargeGeoJSONTileIndex)->Unit(benchmark::kMicrosecond);

static void LargeGeoJSONGetTile(::benchmark::State& state) {
    const std::string json = loadFile("test/fixtures/points.geojson");
    const auto features = mapbox::geojson::parse(json).get<mapbox::geojson::feature_collection>();
    mapbox::geojsonvt::Options options;
    mapbox::geojsonvt::GeoJSONVT index{ features, options };
    for (auto _ : state) {
        index.getTile(12, 1171, 1566);
    }
}
BENCHMARK(LargeGeoJSONGetTile)->Unit(benchmark::kMillisecond)->Iterations(1)->Repetitions(9)->ReportAggregatesOnly(true);

static void LargeGeoJSONToTile(::benchmark::State& state) {
    const std::string json = loadFile("data/countries.geojson");
    const auto features = mapbox::geojson::parse(json).get<mapbox::geojson::feature_collection>();
    for (auto _ : state) {
        mapbox::geojsonvt::geoJSONToTile(features, 12, 1171, 1566, {}, false, true);
    }
}
BENCHMARK(LargeGeoJSONToTile)->Unit(benchmark::kMicrosecond);

static void SingleTileIndex(::benchmark::State& state) {
    const std::string json = loadFile("test/fixtures/single-tile.json");
    const auto features = mapbox::geojson::parse(json);
    mapbox::geojsonvt::Options options;
    options.indexMaxZoom = 7;
    options.indexMaxPoints = 10000;
    mapbox::geojsonvt::GeoJSONVT index{ features, options };
    for (auto _ : state) {
        index.getTile(12, 1171, 1566);
    }
}
BENCHMARK(SingleTileIndex)->Unit(benchmark::kMicrosecond);

static void SingleTileGeoJSONToTile(::benchmark::State& state) {
    const std::string json = loadFile("test/fixtures/single-tile.json");
    const auto features = mapbox::geojson::parse(json);
    for (auto _ : state) {
        mapbox::geojsonvt::geoJSONToTile(features, 12, 1171, 1566, {}, false, true);
    }
}
BENCHMARK(SingleTileGeoJSONToTile)->Unit(benchmark::kMicrosecond);
