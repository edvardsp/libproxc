
BIN_NAME = proxc.out

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
DEP_DIR = deps

TARGET = $(BIN_DIR)/$(BIN_NAME) 

CC = clang

OPT = 
WARN = -Wall -Wextra -Werror
ifeq ($(CC),clang)
WARN += -Weverything -Wno-reserved-id-macro -Wno-gnu-zero-variadic-macro-arguments \
	   -Wno-missing-prototypes -Wno-padded -Wno-switch-enum -Wno-missing-noreturn \
	   -Wno-gnu-union-cast
endif
DEFINES = -D_GNU_SOURCE -DCTX_IMPL
INCLUDES = -I$(SRC_DIR)

CFLAGS = -std=gnu99 $(OPT) $(WARN) $(DEFINES) $(INCLUDES)
LFLAGS = 
LDFLAGS = -pthread

C_FILES = $(shell find $(SRC_DIR) -name "*.c")
O_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

MKDIR_P = mkdir -p
RMDIR_P = rmdir -p
RM_F	= rm -f

.PHONY: all debug release profiling clean 

# Default target
all: debug

# Debug build
debug: OPT += -O0
debug: DEFINES += -DDEBUG
debug: CFLAGS += -g
debug: $(TARGET)

# Release build
release: OPT += -O2
release: DEFINES += -DNDEBUG
release: $(TARGET)

# profiling
profiling: CFLAGS += -g -fno-omit-frame-pointer -fno-inline-functions \
	 -fno-optimize-sibling-calls
profiling: LFLAGS += -g
profiling: release

# Link
$(TARGET): $(O_FILES) 
	@$(MKDIR_P) $(BIN_DIR)
	$(CC) $(LFLAGS) -o $@ $^ $(LDFLAGS)

# Dependency files
DEPS = $(C_FILES:$(SRC_DIR)/%.c=$(DEP_DIR)/%.d)
-include $(DEPS)

# compile and generate dependency info
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEP_DIR)/%.d
	@$(MKDIR_P) $(OBJ_DIR) $(DEP_DIR)
	@$(CC) $(CFLAGS) -MT $@ -MM -MP -MF $(DEP_DIR)/$*.d $<
	$(CC) $(CFLAGS) -o $@ -c $<

$(DEP_DIR)/%.d: ;

# remove compilation products
clean:
	$(RM_F) $(TARGET) $(O_FILES) $(DEPS)

