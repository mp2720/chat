.PHONY: run debug build 

run: build
	build/chat

debug: build
	gdb build/chat

build:
	meson compile -C build/
