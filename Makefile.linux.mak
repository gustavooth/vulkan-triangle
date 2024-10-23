APP = triangle

APP_DIR = app
OBJ_DIR = obj
SHADER_DIR = app/shader

SRC_DIR = src
SHADER_SRC_DIR = shader

SRC = $(shell find $(SRC_DIR) -name '*.c')
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

FRAG_SHADER = $(shell find $(SHADER_SRC_DIR) -name '*.frag')
VERT_SHADER = $(shell find $(SHADER_SRC_DIR) -name '*.vert')
FRAG_SPIRV = $(patsubst $(SHADER_SRC_DIR)/%.frag, $(SHADER_DIR)/%.frag.spv, $(FRAG_SHADER))
VERT_SPIRV = $(patsubst $(SHADER_SRC_DIR)/%.vert, $(SHADER_DIR)/%.vert.spv, $(VERT_SHADER))
SPIRV = $(FRAG_SPIRV) $(VERT_SPIRV)

CC = clang
CFLAGS = -g -Wall
INC_FLAGS = -I$(SRC_DIR) -I/usr/include
LINK_FLAGS = -lwayland-client -lvulkan
DEFINES = -DPLATFORM_WAYLAND

SHADERC = glslc

all: build

build: $(APP_DIR)/$(APP) $(SPIRV)

# Build App
$(APP_DIR)/$(APP): $(OBJ)
	@mkdir -p $(APP_DIR)
	clang $(CFLAGS) $(LINK_FLAGS) $(OBJ) -o $@

# Build objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEFINES) $(INC_FLAGS) -c $< -o $@

# Build shader spir-v
$(SHADER_DIR)/%.spv: $(SHADER_SRC_DIR)/%
	@mkdir -p $(dir $@)
	glslc $< -o $@

run: build
	@cd $(APP_DIR) && ./$(APP)

clean:
	rm -rf $(APP_DIR) $(OBJ_DIR)

.PHONY: all build run clean
