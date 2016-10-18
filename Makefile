
BIN_NAME = proxc.out

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
DEP_DIR = deps

TARGET = $(BIN_DIR)/$(BIN_NAME) 

CC = clang

OPT = -O0
WARN = -Wall -Wextra -Werror
ifeq ($(CC),clang)
WARN += -Weverything -Wno-reserved-id-macro -Wno-gnu-zero-variadic-macro-arguments \
	   -Wno-missing-prototypes -Wno-padded
endif
DEFINES = -D_GNU_SOURCE -DDEBUG
INCLUDES = -I$(SRC_DIR)

CFLAGS = -std=gnu99 -g $(OPT) $(WARN) $(DEFINES) $(INCLUDES)
LDFLAGS = -pthread

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
DEPS = $(C_FILES:$(SRC_DIR)/%.c=$(DEP_DIR)/%.d)
-include $(DEPS)

# compile and generate dependency info
	#@$(CC) -c $(CFLAGS) -MT $@ -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o, $(DEP_DIR)/%.d, $@) $<
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEP_DIR)/%.d
	@$(MKDIR_P) $(OBJ_DIR) $(DEP_DIR)
	@$(CC) $(CFLAGS) -MT $@ -MM -MP -MF $(DEP_DIR)/$*.d $<
	$(CC) $(CFLAGS) -o $@ -c $<

$(DEP_DIR)/%.d: ;

# remove compilation products
clean:
	$(RM_F) $(TARGET) $(O_FILES) $(DEPS)

