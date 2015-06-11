BUILDTYPE ?= Release

all: build

config.gypi:
	./configure

.PHONY: build
build: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make
	make -C build

.PHONY: clean
clean:
	-rm -rf build
	-rm -rf config.gypi
