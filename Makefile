CXXFLAGS += -I include -std=c++14 -Wall -Wextra -O3

MASON ?= .mason/mason

VARIANT = variant 1.1.0
GEOMETRY = geometry 0.7.0
GEOJSON = geojson 0.1.1-cxx03abi
GLFW = glfw 3.1.2

VARIANT_FLAGS = `$(MASON) cflags $(VARIANT)`
GEOMETRY_FLAGS = `$(MASON) cflags $(GEOMETRY)`
GEOJSON_FLAGS = `$(MASON) cflags $(GEOJSON)` `$(MASON) static_libs $(GEOJSON)`
GLFW_FLAGS = `$(MASON) cflags $(GLFW)` `$(MASON) ldflags $(GLFW)` `$(MASON) static_libs $(GLFW)`
BASE_FLAGS = $(VARIANT_FLAGS) $(GEOMETRY_FLAGS) $(GEOJSON_FLAGS)

DEPS = mason_packages include/mapbox/geojsonvt/*.hpp include/mapbox/geojsonvt.hpp bench/util.hpp Makefile

default: bench

mason_packages: Makefile
	$(MASON) install $(VARIANT)
	$(MASON) install $(GEOMETRY)
	$(MASON) install $(GEOJSON)
	$(MASON) install $(GLFW)

build:
	mkdir -p build

build/bench: build bench/run.cpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) bench/run.cpp -o build/bench $(BASE_FLAGS)

build/debug: build debug/debug.cpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) debug/debug.cpp -o build/debug $(BASE_FLAGS) $(GLFW_FLAGS)

debug: build/debug
	./build/debug

bench: build/bench
	./build/bench

format:
	clang-format include/mapbox/geojsonvt/*.hpp include/mapbox/geojsonvt.hpp test/test.cpp debug/debug.cpp bench/*.cpp -i

clean:
	rm -rf build
