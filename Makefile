.PHONY: run debug build setup clean test cov

#to work with a single folder from multiple systems
BUILD_DIR=build/$(shell uname -n)

setup:
	mkdir -p ${BUILD_DIR}/
	meson setup ${BUILD_DIR}/ --buildtype=debug -Db_coverage=true

run: build
	${BUILD_DIR}/src/chat

debug: build
	gdb ${BUILD_DIR}/src/chat

build:
	meson compile -C ${BUILD_DIR}/

clean:
	rm -rf ${BUILD_DIR}/

test: build
	meson test -C ${BUILD_DIR}

cov: test
	mkdir -p coverage
	gcovr -e subprojects -e src/main.cpp --html-details coverage/coverage.html\
		--html-theme github.dark-blue
