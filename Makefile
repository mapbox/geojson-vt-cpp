CXXFLAGS += -I include -std=c++14 -Wall -Wextra -O3

MASON ?= .mason/mason

VARIANT = variant 1.1.0
GEOMETRY = geometry 0.7.0
GEOJSON = geojson 0.1.1-cxx03abi

DEPS = `$(MASON) cflags $(VARIANT)` `$(MASON) cflags $(GEOMETRY)` `$(MASON) cflags $(GEOJSON)` `$(MASON) static_libs $(GEOJSON)`

default: test

mason_packages: Makefile
	$(MASON) install $(VARIANT)
	$(MASON) install $(GEOMETRY)
	$(MASON) install $(GEOJSON)

build:
	mkdir -p build

build/test: build test/test.cpp mason_packages include/mapbox/geojsonvt/*.hpp include/mapbox/geojsonvt.hpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) test/test.cpp -o build/test $(DEPS)

test: build/test
	./build/test

format:
	clang-format include/mapbox/geojsonvt/*.hpp include/mapbox/geojsonvt.hpp test/test.cpp -i

clean:
	rm -rf build
