
BIN_NAME = proxc.out

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
DEPS_DIR = deps

TARGET = $(BIN_DIR)/$(BIN_NAME) 

CC = clang

OPT = -O0
WARN = -Wall -Wextra -Werror 
DEFINES = -D_GNU_SOURCE -DDEBUG
INCLUDES = -I$(SRC_DIR)

CFLAGS = -std=gnu99 -g $(OPT) $(WARN) $(DEFINES) $(INCLUDES) 
LDFLAGS = -pthread -lslog

C_FILES = $(shell find $(SRC_DIR) -name "*.c")
O_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

MKDIR_P = mkdir -p
RMDIR_P = rmdir -p
RM_F	= rm -f

.PHONY: all clean 

# Default target
all: $(TARGET)

# Link
$(TARGET): $(O_FILES)
	@$(MKDIR_P) $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)

# Dependency files
DEPS = $(O_FILES:$(OBJ_DIR)/%.o=$(DEPS_DIR)/%.d)
-include $(DEPS)

# compile and generate dependency info
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR_P) $(OBJ_DIR) $(DEPS_DIR)
	$(CC) $(CFLAGS) -o $@ -c $< 
	@$(CC) -MM ${CFLAGS} $< > $(DEPS_DIR)/$*.d

# remove compilation products
clean:
	$(RM_F) $(TARGET) $(O_FILES) $(DEPS)

