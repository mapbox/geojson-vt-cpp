BUILDTYPE ?= Release

all: lib

config.gypi:
	./configure

.PHONY: lib
lib: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make
	make -C build geojsonvt

.PHONY: xproj
xproj: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f xcode
	open build/geojsonvt.xcodeproj

.PHONY: clean
clean:
	-rm -rf build
	-rm -rf config.gypi
