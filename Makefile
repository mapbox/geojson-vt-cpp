BUILDTYPE ?= Release

all: lib

config.gypi:
	./configure

.PHONY: lib
lib: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make
	make -C build geojsonvt

.PHONY: debug
debug: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make
	make -C build debug

install:
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make
	make -C build install

.PHONY: run-debug
run-debug: debug
	build/${BUILDTYPE}/debug

.PHONY: test
test: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make
	make -C build test
	build/$(BUILDTYPE)/test

.PHONY: xproj
xproj: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f xcode
	open build/geojsonvt.xcodeproj

.PHONY: clean
clean:
	-rm -rf build
	-rm -rf config.gypi
