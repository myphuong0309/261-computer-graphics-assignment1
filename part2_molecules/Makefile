# =============================================================================
# Makefile — Part 2: Atoms & Molecules Visualizer
# HCMUT Computer Graphics, Semester I 2026-2027
#
# Shared code (Shader, Camera, ImGui, GLM) lives in ../common and is shared with Part 1.
# =============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wno-unused-parameter
TARGET   := molecules

# --- Directories ---
SRC_DIR    := src
SHADER_DIR := shaders
COMMON_DIR := ../common
BUILD_DIR  := build
IMGUI_DIR  := $(COMMON_DIR)/vendor/imgui

# --- Include paths ---
# (main.cpp includes "../vendor/imgui/..." - that resolves through -I$(COMMON_DIR)/src)
INCLUDES := \
    -I$(IMGUI_DIR) \
    -I$(COMMON_DIR)/vendor/glm \
    -I$(COMMON_DIR)/src \
    -I$(SRC_DIR)

# --- Libraries ---
LIBS := -lGL -lGLEW -lglfw -lstdc++fs

# --- Source files ---
APP_SRCS    := main Mesh SceneNode AtomData BohrModel MoleculeScene
COMMON_SRCS := Shader Camera
IMGUI_SRCS  := imgui imgui_draw imgui_tables imgui_widgets imgui_impl_glfw imgui_impl_opengl3

OBJS := $(patsubst %,$(BUILD_DIR)/%.o,$(APP_SRCS)) \
        $(patsubst %,$(BUILD_DIR)/common/%.o,$(COMMON_SRCS)) \
        $(patsubst %,$(BUILD_DIR)/imgui/%.o,$(IMGUI_SRCS))

# =============================================================================
.PHONY: all clean run

all: $(BUILD_DIR)/$(TARGET)
	@echo ""
	@echo "=== Build successful! ==="
	@echo "Run with: make run"
	@echo "Or:       ./$(BUILD_DIR)/$(TARGET)"

$(BUILD_DIR)/$(TARGET): $(OBJS)
	@echo "Linking $@..."
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/common/%.o: $(COMMON_DIR)/src/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (common) $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/imgui/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling (ImGui) $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run from the project root so shaders/ is found
run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned."

# Rebuild everything
rebuild: clean all
