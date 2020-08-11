CXXFLAGS += -I include -std=c++14 -Wall -Wextra -D_GLIBCXX_USE_CXX11_ABI=0
RELEASE_FLAGS ?= -O3 -DNDEBUG -g -ggdb3
DEBUG_FLAGS ?= -g -O0 -DDEBUG

MASON ?= .mason/mason

VARIANT = variant 1.1.5
GEOMETRY = geometry 1.0.0
GEOJSON = geojson 0.4.3
GLFW = glfw 3.1.2
GTEST = gtest 1.8.0
RAPIDJSON = rapidjson 1.1.0
BENCHMARK = benchmark 1.4.1

VARIANT_FLAGS = `$(MASON) cflags $(VARIANT)`
GEOMETRY_FLAGS = `$(MASON) cflags $(GEOMETRY)`
GEOJSON_FLAGS = `$(MASON) cflags $(GEOJSON)`
GLFW_FLAGS = `$(MASON) cflags $(GLFW)` `$(MASON) static_libs $(GLFW)` `$(MASON) ldflags $(GLFW)`
GTEST_FLAGS = `$(MASON) cflags $(GTEST)` `$(MASON) static_libs $(GTEST)` `$(MASON) ldflags $(GTEST)`
RAPIDJSON_FLAGS = `$(MASON) cflags $(RAPIDJSON)`
BASE_FLAGS = $(VARIANT_FLAGS) $(GEOMETRY_FLAGS) $(GEOJSON_FLAGS)
BENCHMARK_FLAGS = `$(MASON) cflags $(BENCHMARK)` `$(MASON) static_libs $(BENCHMARK)` `$(MASON) ldflags $(BENCHMARK)`

DEPS = mason_packages/headers/geometry include/mapbox/geojsonvt/*.hpp include/mapbox/geojsonvt.hpp bench/util.hpp Makefile

default: test

mason_packages/headers/geometry: Makefile
	$(MASON) install $(VARIANT)
	$(MASON) install $(GEOMETRY)
	$(MASON) install $(GEOJSON)
	$(MASON) install $(GLFW)
	$(MASON) install $(GTEST)
	$(MASON) install $(RAPIDJSON)
	$(MASON) install $(BENCHMARK)

build:
	mkdir -p build

build/bench: build bench/main.cpp bench/benchmark.cpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(RELEASE_FLAGS) bench/main.cpp bench/benchmark.cpp -o build/bench $(BASE_FLAGS) $(RAPIDJSON_FLAGS) $(BENCHMARK_FLAGS)

build/debug: build debug/debug.cpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) debug/debug.cpp -o build/debug $(BASE_FLAGS) $(GLFW_FLAGS) $(RAPIDJSON_FLAGS)

build/test: build test/*.cpp test/*.hpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEBUG_FLAGS) test/test.cpp test/util.cpp -o build/test $(BASE_FLAGS) $(GTEST_FLAGS) $(RAPIDJSON_FLAGS)

bench: build/bench
	./build/bench

debug: build/debug
	./build/debug

test: build/test
	./build/test

format:
	clang-format include/mapbox/geojsonvt/*.hpp include/mapbox/geojsonvt.hpp test/*.cpp test/*.hpp debug/debug.cpp bench/*.cpp -i

clean:
	rm -rf build
