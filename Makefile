CXXFLAGS += -I include -std=c++14 -Wall -Wextra -D_GLIBCXX_USE_CXX11_ABI=0
RELEASE_FLAGS ?= -O3 -DNDEBUG
DEBUG_FLAGS ?= -g -O0 -DDEBUG

MASON ?= .mason/mason

VARIANT = variant 1.1.5
GEOMETRY = geometry 0.9.3
GEOJSON = geojson 0.4.2
GLFW = glfw 3.1.2
GTEST = gtest 1.8.0
RAPIDJSON = rapidjson 1.1.0
JNIHPP = jni.hpp 4.0.1
VTZERO = vtzero 1.0.3
PROTOZERO = protozero 1.6.3

VARIANT_FLAGS = `$(MASON) cflags $(VARIANT)`
GEOMETRY_FLAGS = `$(MASON) cflags $(GEOMETRY)`
GEOJSON_FLAGS = `$(MASON) cflags $(GEOJSON)`
GLFW_FLAGS = `$(MASON) cflags $(GLFW)` `$(MASON) static_libs $(GLFW)` `$(MASON) ldflags $(GLFW)`
GTEST_FLAGS = `$(MASON) cflags $(GTEST)` `$(MASON) static_libs $(GTEST)` `$(MASON) ldflags $(GTEST)`
RAPIDJSON_FLAGS = `$(MASON) cflags $(RAPIDJSON)`
BASE_FLAGS = $(VARIANT_FLAGS) $(GEOMETRY_FLAGS) $(GEOJSON_FLAGS)

JAVA_HOME=$(shell /usr/libexec/java_home)
JNIHPP_FLAGS = `$(MASON) cflags $(JNIHPP)` -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/darwin
VTZERO_FLAGS = `$(MASON) cflags $(VTZERO)`
PROTOZERO_FLAGS = `$(MASON) cflags $(PROTOZERO)`

TILER_CLASS_NAME_FLAGS = -DTILER_CLASS_NAME=\"$(TILER_KLASS)\"\;

DEPS = mason_packages/headers/geometry include/mapbox/geojsonvt/*.hpp include/mapbox/geojsonvt.hpp bench/util.hpp Makefile

default: test

mason_packages/headers/geometry: Makefile
	$(MASON) install $(VARIANT)
	$(MASON) install $(GEOMETRY)
	$(MASON) install $(GEOJSON)
	$(MASON) install $(GLFW)
	$(MASON) install $(GTEST)
	$(MASON) install $(RAPIDJSON)
	$(MASON) install $(JNIHPP)
	$(MASON) install $(VTZERO)
	$(MASON) install $(PROTOZERO)

build:
	mkdir -p build

build/bench: build bench/run.cpp $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(RELEASE_FLAGS) bench/run.cpp -o build/bench $(BASE_FLAGS) $(RAPIDJSON_FLAGS)

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

jni: build $(DEPS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(RELEASE_FLAGS) $(JNIHPP_FLAGS) $(VTZERO_FLAGS) $(PROTOZERO_FLAGS) $(TILER_CLASS_NAME_FLAGS) $(BASE_FLAGS) $(RAPIDJSON_FLAGS) jni/tiler.cpp -dynamiclib -o build/libtiler.jnilib

