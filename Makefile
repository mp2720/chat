.PHONY: run debug build setup clean

#to work with a single folder from multiple systems
BUILD_DIR=build/$(shell uname -n)

setup:
	mkdir -p ${BUILD_DIR}/
	meson setup ${BUILD_DIR}/ --buildtype=debug

run: build
	${BUILD_DIR}/chat

debug: build
	gdb ${BUILD_DIR}/chat

build:
	meson compile -C ${BUILD_DIR}/

clean:
	rm -rf ${BUILD_DIR}/
