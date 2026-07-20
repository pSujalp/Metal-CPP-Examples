

CXX := clang++
CC := clang
CPPFLAGS := -I./metal-cpp -I./metal-cpp-extensions -I./include
CXXFLAGS := -Wall -std=c++23 -O2 -fno-objc-arc
CFLAGS := -Wall -std=c11 -O2
LDFLAGS := -framework Metal -framework Foundation -framework Cocoa -framework CoreGraphics -framework MetalKit

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

copy-shaders: build/shaders/square.metal
copy-assets: build/assets/mc_grass.jpeg

$(TARGET): $(OBJ) copy-shaders copy-assets
	$(CXX) $(CXXFLAGS) $(OBJ) $(LDFLAGS) -o $@

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


