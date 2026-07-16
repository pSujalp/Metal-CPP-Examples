CXX := clang++
CC  := clang



GLM_PREFIX := $(shell brew --prefix glm)


BUILD_DIR := build
TARGET := $(BUILD_DIR)/metal

EXTERNAL := external





GLFW_PREFIX   := $(shell brew --prefix glfw)





CPPFLAGS := \
	-Iinclude \
	-I$(EXTERNAL)/metal-cpp \
	-I$(EXTERNAL)/metal-cpp-extensions \
	-I$(EXTERNAL)/imgui/include \
	-I$(EXTERNAL)/imgui/src \
	-I$(GLFW_PREFIX)/include \
	-I$(EXTERNAL)/stb \
	-MMD -MP \
	-I$(GLM_PREFIX)/include






CFLAGS := \
	-Wall \
	-std=c11 \
	-O2


CXXFLAGS := \
	-Wall \
	-std=c++23 \
	-O2


OBJCXXFLAGS := \
	$(CXXFLAGS) \
	-fno-objc-arc






LDFLAGS := \
	-Llib \
	-L$(GLFW_PREFIX)/lib \
	-framework Metal \
	-framework MetalKit \
	-framework Foundation \
	-framework Cocoa \
	-framework CoreGraphics \
	-framework ModelIO \
	-framework MetalPerformanceShaders \
	-framework QuartzCore


LDLIBS := \
	-lglfw






SRC_C := $(wildcard src/*.c)

SRC_CPP := $(wildcard src/*.cpp)

SRC_MM := $(wildcard src/*.mm)


IMGUI_CPP := $(wildcard $(EXTERNAL)/imgui/src/*.cpp)

IMGUI_MM := $(wildcard $(EXTERNAL)/imgui/src/*.mm)






OBJ := \
	$(patsubst src/%.c,$(BUILD_DIR)/%.c.o,$(SRC_C)) \
	$(patsubst src/%.cpp,$(BUILD_DIR)/%.cpp.o,$(SRC_CPP)) \
	$(patsubst src/%.mm,$(BUILD_DIR)/%.mm.o,$(SRC_MM)) \
	$(patsubst $(EXTERNAL)/imgui/src/%.cpp,$(BUILD_DIR)/imgui/%.cpp.o,$(IMGUI_CPP)) \
	$(patsubst $(EXTERNAL)/imgui/src/%.mm,$(BUILD_DIR)/imgui/%.mm.o,$(IMGUI_MM))


DEPS := $(OBJ:.o=.d)







.DEFAULT_GOAL := all

.PHONY: all clean run


all: $(TARGET)







$(BUILD_DIR):
	mkdir -p $@


build/default.air: include/PBR.metal | $(BUILD_DIR)
	xcrun -sdk macosx metal -c $< -o $@


build/default.metallib: build/default.air
	xcrun -sdk macosx metallib $< -o $@



build/skybox.air: include/Skybox.metal | $(BUILD_DIR)
	xcrun -sdk macosx metal -c $< -o $@


build/skybox.metallib: build/skybox.air
	xcrun -sdk macosx metallib $< -o $@

build/assets/%: assets/%
	mkdir -p $(dir $@)
	cp $< $@

copy-assets2 : build/assets/newport_loft.hdr
copy-assets3 : build/assets/RUST/albedo.png
copy-assets4 : build/assets/RUST/ao.png
copy-assets5 : build/assets/RUST/metallic.png
copy-assets6 : build/assets/RUST/normal.png
copy-assets7 : build/assets/RUST/roughness.png


$(TARGET): $(OBJ) build/default.metallib build/skybox.metallib copy-assets2 copy-assets3 copy-assets4 copy-assets5 copy-assets6 copy-assets7
	$(CXX) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@



$(BUILD_DIR)/%.c.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@







$(BUILD_DIR)/%.cpp.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@







$(BUILD_DIR)/%.mm.o: src/%.mm
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(OBJCXXFLAGS) -c $< -o $@






$(BUILD_DIR)/imgui/%.cpp.o: $(EXTERNAL)/imgui/src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@







$(BUILD_DIR)/imgui/%.mm.o: $(EXTERNAL)/imgui/src/%.mm
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(OBJCXXFLAGS) -c $< -o $@







run: all
	./$(TARGET)







clean:
	rm -rf $(BUILD_DIR)







-include $(DEPS)