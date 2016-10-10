
TARGET = bin/proxc.out

BINDIR = bin
OBJDIR = obj
DEPSDIR = deps

CC = clang

OPT = -O0
WARN = -Wall -Wextra -Werror 
DEFINES = -D_GNU_SOURCE -DDEBUG
INCLUDES = -Isrc

CFLAGS = -std=gnu99 -g $(OPT) $(WARN) $(DEFINES) $(INCLUDES) 
LDFLAGS = -pthread -lslog

C_FILES = $(shell find src -name "*.c")
O_FILES = $(C_FILES:src/%.c=obj/%.o)
MKDIR_P = mkdir -p

.PHONY: all clean 

all: directories $(TARGET)

directories: $(BINDIR) $(OBJDIR) $(DEPSDIR) 
$(BINDIR):
	mkdir -p $(BINDIR)
$(OBJDIR):
	mkdir -p $(OBJDIR)
$(DEPSDIR):
	mkdir -p $(DEPSDIR)

$(TARGET): $(O_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.c $(DEPSDIR)/%.d
	$(CC) $(CFLAGS) -o $@ -c $< 

deps/%.d: src/%.c
	${CC} ${CFLAGS} -MM -MT $(patsubst src/%.c, deps/%.d, $<) $< -MF $@

DEPS = $(patsubst src/%.c, deps/%.d, $(C_FILES))
-include $(DEPS)

clean:
	rm -f $(TARGET) $(O_FILES) $(DEPS)


