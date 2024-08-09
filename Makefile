.PHONY: run run_echo run_echo_opus debug build setup clean test cov

#to work with a single folder from multiple systems
BUILD_DIR=build

setup:
	mkdir -p ${BUILD_DIR}/
	meson setup ${BUILD_DIR}/ --buildtype=debug -Db_coverage=true

run: build
	${BUILD_DIR}/src/demo

run_server: build
	${BUILD_DIR}/src/demo_server

debug: build
	gdb ${BUILD_DIR}/src/chat

build:
	meson compile -C ${BUILD_DIR}/

clean:
	rm -rf ${BUILD_DIR}/
