# =========================
# Project settings
# =========================
TARGET        := ce_engine
BUILD_DIR     := build
ASSET_FOLDER  := assets
SCRIPT_FOLDER := scripts
SRC_DIR       := source
INCLUDE_DIR   := include
DATA_FILE_NAME := data.tcf
MAKEFLAGS += -j4
EXE := $(TARGET)

# =========================
# Detect OS
# =========================
ifeq ($(OS),Windows_NT)
	HOST_OS := Windows
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		HOST_OS := Linux
	endif
	ifeq ($(UNAME_S),Darwin)
		HOST_OS := macOS
	endif
endif

ifeq ($(strip $(HOST_OS)),)
	HOST_OS := Unknown OS
endif

HOST_OS_ESCAPED := "\"$(HOST_OS)\""

# =========================
# Compiler
# =========================
CC  := clang
CXX := clang++

ifeq ($(OS),Windows_NT)
	EXE := $(TARGET).exe
	PLATFORM_FLAGS := -DPLATFORM_WINDOWS
else
	PLATFORM_FLAGS :=
endif

# =========================
# Sources
# =========================
CPP_SRC := $(shell find $(SRC_DIR) -name '*.cpp')
C_SRC   := $(shell find $(SRC_DIR) -name '*.c')

CPP_OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SRC))
C_OBJ   := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRC))
OBJ := $(CPP_OBJ) $(C_OBJ)

# =========================
# Original C / C++ flags (preserved)
# =========================
CFLAGS := -Wall -Wextra -I$(INCLUDE_DIR) -I$(INCLUDE_DIR)/third_party -I$(INCLUDE_DIR)/third_party/lua

CXXFLAGS := -Wall -Wextra -std=c++20 \
    -I$(INCLUDE_DIR)/third_party \
	-I$(INCLUDE_DIR)/third_party/imgui \
	-I$(INCLUDE_DIR)/third_party/lua \
    -I$(INCLUDE_DIR) \
    $(PLATFORM_FLAGS) \
    -DENGINE_BUILT_ON_OS=$(HOST_OS_ESCAPED) \
	-DCE_DATA_FILE_NAME=\"$(DATA_FILE_NAME)\"

# =========================
# Linker flags
# =========================
ifeq ($(OS),Windows_NT)
	LDFLAGS := -lSDL3 -lSDL3_image -lraylib -lopengl32 -lole32 -luuid -lcomdlg32 -limm32 -loleaut32 -Iinclude/third_party/imgui -Iinclude/third_party/lua 
else
	LDFLAGS := -lSDL3 -lSDL3_image -lraylib -lm -lGL -lX11 -lpthread -ldl -lrt -lXi -Iinclude/third_party/imgui -Iinclude/third_party/lua 
endif

# Windows GUI subsystem
ifeq ($(OS),Windows_NT)
	ifeq ($(filter debug,$(MAKECMDGOALS)),)
		SUBSYSTEM_FLAG := -Wl,-subsystem,windows
	else
		SUBSYSTEM_FLAG :=
	endif
endif

# =========================
# Asset packing
# =========================
TCF_CMD := python $(SCRIPT_FOLDER)/tcf.py pack $(ASSET_FOLDER) data.tcf

# =========================
# Rules
# =========================
all: $(EXE)

$(EXE): $(OBJ)
	@echo "Packing assets..."
	@$(TCF_CMD)
	@echo "Linking executable on $(HOST_OS)..."
	$(CXX) -o $@ $(OBJ) $(LDFLAGS) $(SUBSYSTEM_FLAG)

# Compile C++ (with original flags)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C (with original flags)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# =========================
# Run
# =========================
run: $(EXE)
ifeq ($(OS),Windows_NT)
	@echo "Run manually: $(EXE)"
else
	./$(EXE)
endif

# =========================
# Debug
# =========================
debug: CXXFLAGS += -g
debug: CFLAGS += -g
debug: clean all

# =========================
# Clean
# =========================
clean:
	rm -rf $(BUILD_DIR) $(EXE)


# =========================
# Graphics Backend (Linux)
# =========================
GRAPHICS_BACKEND ?= X11  # default

ifeq ($(GRAPHICS_BACKEND),X11)
	CXXFLAGS += -DGRAPHICS_BACKEND_X11
endif
ifeq ($(GRAPHICS_BACKEND),WAYLAND)
	CXXFLAGS += -DGRAPHICS_BACKEND_WAYLAND
endif
gcc:
	$(MAKE) CC=gcc CXX=g++

clang:
	$(MAKE) CC=clang CXX=clang++

assets:
	rm $(DATA_FILE_NAME)
	@echo "Packing assets only..."
	@$(TCF_CMD)

# Build executable only (skip asset packing)
exe_only: $(OBJ)
	@echo "Linking executable on $(HOST_OS)..."
	$(CXX) -o $(EXE) $(OBJ) $(LDFLAGS) $(SUBSYSTEM_FLAG)

.PHONY: all run debug clean gcc clang assets exe_only
