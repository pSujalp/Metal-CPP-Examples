CXX := clang++
CC := clang


# External libraries
EXTERNAL := external
CPPFLAGS := \
    -I./include \
    -I$(EXTERNAL)/metal-cpp \
    -I$(EXTERNAL)/metal-cpp-extensions \
	-I$(EXTERNAL)/GLFW

CXXFLAGS := -Wall -std=c++23 -O2 -fno-objc-arc
CFLAGS := -Wall -std=c11 -O2

CPPFLAGS += -I$(shell brew --prefix glfw)/include

LDFLAGS += \
    -L$(shell brew --prefix glfw)/lib \
    -framework Metal \
    -framework Foundation \
    -framework Cocoa \
    -framework CoreGraphics \
    -framework MetalKit \
    -framework ModelIO \
    -framework MetalPerformanceShaders \
	-framework QuartzCore

LDLIBS += -lglfw

TARGET := build/metal
SRC_C   := $(wildcard src/*.c)
SRC_CPP := $(wildcard src/*.cpp)
SRC_MM  := $(wildcard src/*.mm)
OBJ := \
    $(patsubst src/%.c,build/%.c.o,$(SRC_C)) \
    $(patsubst src/%.cpp,build/%.cpp.o,$(SRC_CPP)) \
    $(patsubst src/%.mm,build/%.mm.o,$(SRC_MM))

SHADERS := $(patsubst shaders/%,build/shaders/%,$(wildcard shaders/*))
ASSETS  := $(patsubst assets/%,build/assets/%,$(wildcard assets/*))

LIB_D := -Llib/

.DEFAULT_GOAL := all
.PHONY: all clean run
.SECONDARY:

all: $(TARGET) $(SHADERS) $(ASSETS)

build/shaders/%: shaders/%
	mkdir -p $(dir $@)
	cp $< $@

build/assets/%: assets/%
	mkdir -p $(dir $@)
	cp $< $@

$(TARGET): $(OBJ) $(SHADERS) $(ASSETS)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LDFLAGS) $(LIB_D) $(LDLIBS) -o $@

build/%.c.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.cpp.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

build/%.mm.o: src/%.mm
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf build