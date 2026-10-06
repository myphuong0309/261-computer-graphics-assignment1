# =============================================================================
# Makefile - Part 1: Drawing Basic Shapes
# HCMUT Computer Graphics, Semester I 2026-2027
#
# Shared code lives in ../common (also used by Part 2):
#   - src/Shader.cpp, src/Camera.cpp   (compiled into build/common/)
#   - vendor/imgui, vendor/glm         (compiled / included from there)
# =============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wno-unused-parameter
TARGET   := shapes

COMMON_DIR := ../common
SRC_DIR    := src
BUILD_DIR  := build
IMGUI_DIR  := $(COMMON_DIR)/vendor/imgui

INCLUDES := -I$(IMGUI_DIR) -I$(COMMON_DIR)/vendor/glm -I$(COMMON_DIR)/src -I$(SRC_DIR)
LIBS     := -lGL -lGLEW -lglfw

# --- Part 1 sources (the library part is shared with the tests) ---
CORE_SRCS := Geometry ExprParser Surface ModelLoader
APP_SRCS  := main Gui Scene SceneObject ShapeMesh Texture Screenshot Paths

# --- Shared code (../common) ---
COMMON_SRCS := Shader Camera
IMGUI_SRCS  := imgui imgui_draw imgui_tables imgui_widgets imgui_impl_glfw imgui_impl_opengl3

OBJS := $(patsubst %,$(BUILD_DIR)/%.o,$(CORE_SRCS) $(APP_SRCS)) \
        $(patsubst %,$(BUILD_DIR)/common/%.o,$(COMMON_SRCS)) \
        $(patsubst %,$(BUILD_DIR)/imgui/%.o,$(IMGUI_SRCS))

TEST_OBJS := $(patsubst %,$(BUILD_DIR)/%.o,$(CORE_SRCS))

.PHONY: all run test clean rebuild

all: $(BUILD_DIR)/$(TARGET)
	@echo "=== Build successful! Run with: make run  (or ./$(BUILD_DIR)/$(TARGET)) ==="

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(BUILD_DIR)/common/%.o: $(COMMON_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (common) $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/imgui/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (ImGui) $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run from this folder so shaders/ and assets/ are found
run: all
	./$(BUILD_DIR)/$(TARGET)

# GL-free unit tests: expression parser, model loaders, shape generators
test: $(BUILD_DIR)/test_core
	./$(BUILD_DIR)/test_core

$(BUILD_DIR)/test_core: tests/test_core.cpp $(TEST_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned."

rebuild: clean all

-include $(OBJS:.o=.d)
