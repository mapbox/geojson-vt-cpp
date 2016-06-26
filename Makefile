CXXFLAGS += -I include -std=c++14 -Wall -Wextra -O3

MASON ?= .mason/mason

VARIANT = variant 1.1.0
GEOMETRY = geometry 0.7.0
GEOJSON = geojson 0.1.2-cxx03abi
GLFW = glfw 3.1.2
GTEST = gtest 1.7.0
RAPIDJSON = rapidjson 1.0.2

VARIANT_FLAGS = `$(MASON) cflags $(VARIANT)`
GEOMETRY_FLAGS = `$(MASON) cflags $(GEOMETRY)`
GEOJSON_FLAGS = `$(MASON) cflags $(GEOJSON)` `$(MASON) static_libs $(GEOJSON)`
GLFW_FLAGS = `$(MASON) cflags $(GLFW)` `$(MASON) ldflags $(GLFW)` `$(MASON) static_libs $(GLFW)`
GTEST_FLAGS = `$(MASON) cflags $(GTEST)` `$(MASON) ldflags $(GTEST)` `$(MASON) static_libs $(GTEST)`
RAPIDJSON_FLAGS = `$(MASON) cflags $(RAPIDJSON)`
BASE_FLAGS = $(VARIANT_FLAGS) $(GEOMETRY_FLAGS) $(GEOJSON_FLAGS)

DEPS = mason_packages/headers/geometry include/mapbox/geojsonvt/*.hpp include/mapbox/geojsonvt.hpp bench/util.hpp Makefile

default: test

mason_packages/headers/geometry: Makefile
	$(MASON) install $(VARIANT)
	$(MASON) install $(GEOMETRY)
	$(MASON) install $(GEOJSON)
	$(MASON) install $(GLFW)
	$(MASON) install $(GTEST)
	$(MASON) install $(RAPIDJSON)

build:
	mkdir -p build

build/bench: build bench/run.cpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) bench/run.cpp -o build/bench $(BASE_FLAGS)

build/debug: build debug/debug.cpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) debug/debug.cpp -o build/debug $(BASE_FLAGS) $(GLFW_FLAGS)

build/test: build test/*.cpp test/*.hpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) test/test.cpp test/util.cpp -o build/test $(BASE_FLAGS) $(GTEST_FLAGS) $(RAPIDJSON_FLAGS)

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
