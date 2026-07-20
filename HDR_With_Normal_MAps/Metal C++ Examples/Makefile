APP_NAME := MetalTutorial
BUILD_DIR := build
APP_BUNDLE := $(BUILD_DIR)/$(APP_NAME).app
CONTENTS_DIR := $(APP_BUNDLE)/Contents
MACOS_DIR := $(CONTENTS_DIR)/MacOS
RESOURCES_DIR := $(CONTENTS_DIR)/Resources
OBJ_DIR := $(BUILD_DIR)/obj

SRC_DIR := src
METAL_CPP_DIR := metal-cpp
GLFW_DIR := external/GLFW
GLFW_LIB := $(GLFW_DIR)/lib-universal/libglfw3.a
STB_DIR := external/stb
ASSETS_DIR := assets

CXX := xcrun -sdk macosx clang++
METAL := xcrun -sdk macosx metal
METALLIB := xcrun -sdk macosx metallib

CPPFLAGS := -I$(METAL_CPP_DIR) -I$(GLFW_DIR)/include -I$(SRC_DIR) -I$(STB_DIR)
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -g
MMFLAGS := -fobjc-arc
LDFLAGS := $(GLFW_LIB) -framework Cocoa -framework QuartzCore -framework Metal -framework IOKit -framework CoreVideo

APP_OBJECTS := $(OBJ_DIR)/main.o $(OBJ_DIR)/mtl_engine.o $(OBJ_DIR)/mtl_implementation.o
APP_OBJECTS += $(OBJ_DIR)/Texture.o $(OBJ_DIR)/stb_image.o

METAL_SOURCE := $(SRC_DIR)/triangle.metal
METAL_AIR := $(BUILD_DIR)/default.air
METAL_LIB := $(RESOURCES_DIR)/default.metallib
INFO_PLIST := $(CONTENTS_DIR)/Info.plist
BUILD_ASSETS := $(BUILD_DIR)/assets
BUNDLE_ASSETS := $(RESOURCES_DIR)/assets

.PHONY: all run clean

all: $(APP_BUNDLE) $(BUILD_ASSETS) $(BUNDLE_ASSETS)

run: $(APP_BUNDLE) $(BUILD_ASSETS)
	cd $(BUILD_DIR) && ./$(APP_NAME).app/Contents/MacOS/$(APP_NAME)

$(APP_BUNDLE): $(METAL_LIB) $(INFO_PLIST) $(MACOS_DIR)/$(APP_NAME)

$(MACOS_DIR)/$(APP_NAME): $(APP_OBJECTS) | $(MACOS_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(APP_OBJECTS) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.mm | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(MMFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(STB_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(METAL_AIR): $(METAL_SOURCE) | $(BUILD_DIR)
	$(METAL) -c $< -o $@

$(METAL_LIB): $(METAL_AIR) | $(RESOURCES_DIR)
	$(METALLIB) $< -o $@

$(BUILD_ASSETS): $(ASSETS_DIR) | $(BUILD_DIR)
	rm -rf $@
	ditto $< $@

$(BUNDLE_ASSETS): $(ASSETS_DIR) | $(RESOURCES_DIR)
	rm -rf $@
	ditto $< $@

$(INFO_PLIST): | $(CONTENTS_DIR)
	@printf '%s\n' \
		'<?xml version="1.0" encoding="UTF-8"?>' \
		'<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' \
		'<plist version="1.0">' \
		'<dict>' \
		'  <key>CFBundleExecutable</key>' \
		'  <string>$(APP_NAME)</string>' \
		'  <key>CFBundleIdentifier</key>' \
		'  <string>com.example.$(APP_NAME)</string>' \
		'  <key>CFBundleName</key>' \
		'  <string>$(APP_NAME)</string>' \
		'  <key>CFBundlePackageType</key>' \
		'  <string>APPL</string>' \
		'  <key>CFBundleShortVersionString</key>' \
		'  <string>1.0</string>' \
		'  <key>CFBundleVersion</key>' \
		'  <string>1</string>' \
		'</dict>' \
		'</plist>' > $@

$(BUILD_DIR) $(OBJ_DIR) $(CONTENTS_DIR) $(MACOS_DIR) $(RESOURCES_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

-include $(APP_OBJECTS:.o=.d)
