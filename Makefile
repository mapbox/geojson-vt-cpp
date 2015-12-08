BUILDTYPE ?= Release

ifeq ($(shell uname -s), Darwin)
export BUILD = osx
else ifeq ($(shell uname -s), Linux)
export BUILD = linux
else
$(error Cannot determine build platform)
endif


CLANG_TIDY ?= clang-tidy

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

.PHONY: tidy
tidy: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. -Gconfig=$(BUILDTYPE) --generator-output=./build -f ninja
	deps/ninja/ninja-$(BUILD) -C build/$(BUILDTYPE) -t compdb cc cc_s cxx objc objcxx > build/$(BUILDTYPE)/compile_commands.json
	(cd build/$(BUILDTYPE) && git ls-files '../../*.cpp' | xargs -I{} $(CLANG_TIDY) -checks=* {} )

.PHONY: clean
clean:
	-rm -rf build
	-rm -rf config.gypi
