

CXX := clang++
CC := clang
CPPFLAGS := -I./metal-cpp -I./metal-cpp-extensions -I./include
CXXFLAGS := -Wall -std=c++23 -O2 -fno-objc-arc
CFLAGS := -Wall -std=c11 -O2
LDFLAGS := -framework Metal -framework Foundation -framework Cocoa -framework CoreGraphics -framework MetalKit -framework IOKit -framework QuartzCore -framework CoreVideo

TARGET := build/metal

SRC_C := $(wildcard src/*.c)
SRC_CPP := $(wildcard src/*.cpp)
SRC_MM := $(wildcard src/*.mm)
OBJ := $(patsubst src/%.c,build/%.c.o,$(SRC_C)) $(patsubst src/%.cpp,build/%.cpp.o,$(SRC_CPP)) $(patsubst src/%.mm,build/%.mm.o,$(SRC_MM))

.DEFAULT_GOAL := all

.PHONY: all clean run copy-shaders copy-assets
.SECONDARY:

all: $(TARGET)

build/shaders/%: shaders/%
	mkdir -p $(dir $@)
	cp $< $@

build/assets/%: assets/%
	mkdir -p $(dir $@)
	cp $< $@

SKYBOX_ASSETS := $(wildcard assets/skybox/*.jpg)
BUILD_SKYBOX_ASSETS := $(patsubst assets/skybox/%.jpg,build/assets/skybox/%.jpg,$(SKYBOX_ASSETS))

copy-shaders: build/shaders/square.metal
copy-assets: build/assets/mc_grass.jpeg $(BUILD_SKYBOX_ASSETS)
copy-shaders1 : build/shaders/skybox.metal
copy-assets1 : build/assets/skybox.png

build/assets/skybox/%.jpg: assets/skybox/%.jpg
	mkdir -p $(dir $@)
	cp $< $@

$(TARGET): $(OBJ) copy-shaders copy-assets copy-shaders1 copy-assets1
	mkdir -p $(dir $@)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

build/%.c.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.cpp.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

build/%.mm.o: src/%.mm
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	cd build && ./metal

clean:
	rm -rf build


