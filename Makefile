BUILDTYPE ?= Release
ifeq ($(MASON_PLATFORM), linux)
GYP_FLAVOR_SUFFIX ?= -linux
else
GYP_FLAVOR_SUFFIX ?=
endif

ifeq ($(shell uname -s), Darwin)
export BUILD = osx
else ifeq ($(shell uname -s), Linux)
export BUILD = linux
else
$(error Cannot determine build platform)
endif


CLANG_TIDY ?= clang-tidy
CLANG_FORMAT ?= clang-format

all: lib

config.gypi:
	./configure

.PHONY: lib
lib: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make$(GYP_FLAVOR_SUFFIX)
	make -C build geojsonvt

.PHONY: debug
debug: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make$(GYP_FLAVOR_SUFFIX)
	make -C build debug

install: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make$(GYP_FLAVOR_SUFFIX)
	make -C build install

.PHONY: run-debug
run-debug: debug
	build/${BUILDTYPE}/debug

.PHONY: test
test: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f make$(GYP_FLAVOR_SUFFIX)
	make -C build test
	build/$(BUILDTYPE)/test

.PHONY: xproj
xproj: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. --generator-output=./build -f xcode$(GYP_FLAVOR_SUFFIX)
	open build/geojsonvt.xcodeproj

.PHONY: tidy
tidy: config.gypi
	deps/run_gyp geojsonvt.gyp -Iconfig.gypi --depth=. -Goutput_dir=. -Gconfig=$(BUILDTYPE) --generator-output=./build -f ninja
	deps/ninja/ninja-$(BUILD) -C build/$(BUILDTYPE) -t compdb cc cc_s cxx objc objcxx > build/$(BUILDTYPE)/compile_commands.json
	@command -v $(CLANG_TIDY) >/dev/null 2>&1 || { echo >&2 "Can't execute $(CLANG_TIDY). Set CLANG_TIDY to the correct executable or add clang-tidy to your PATH."; exit 1; }
	(cd build/$(BUILDTYPE) && git ls-files '../../*.cpp' | xargs -t -n 1 $(CLANG_TIDY) -checks=*)

.PHONY: format
format:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo >&2 "Can't execute $(CLANG_FORMAT). Set CLANG_FORMAT to the correct executable or add clang-format to your PATH."; exit 1; }
	git ls-files *.cpp *.hpp | xargs -t -n 1 $(CLANG_FORMAT) -i

.PHONY: clean
clean:
	-rm -rf build
	-rm -rf config.gypi
