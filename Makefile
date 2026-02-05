CC      ?= gcc
CSTD    := -std=c11
WARN    := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wdouble-promotion \
           -Wformat=2 -Wundef -fno-common -fstack-protector-strong
OPT     := -O2
DEBUG   := -g3 -DDEBUG

BUILD_DIR := build
BIN_DIR   := $(BUILD_DIR)/bin
OBJ_DIR   := $(BUILD_DIR)/obj
INC_FLAGS := -Iinc -Iinc/lexer -Iinc/vent -Iinc/parser
CFLAGS    := $(CSTD) $(WARN) $(INC_FLAGS) -MMD -MP
LDFLAGS   := 
TARGET    := $(BIN_DIR)/terra

SRCS := $(shell find src -name "*.c")
OBJS := $(SRCS:%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

VALGRIND := valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
LINTER   := cppcheck --enable=all --suppress=missingIncludeSystem --error-exitcode=1
TEST_FILE := test/main.rr 

.PHONY: all clean debug run memcheck lint

all: CFLAGS += $(OPT)
all: $(TARGET)

debug: CFLAGS += $(DEBUG)
debug: $(TARGET)

run: debug
	@echo "[i] Running $(TARGET)"
	@./$(TARGET) $(TEST_FILE) --lexer-debug --parser-debug

memcheck: debug
	@echo "[i] Running Valgrind"
	@$(VALGRIND) ./$(TARGET) $(TEST_FILE) --lexer-debug --parser-debug

lint:
	@echo "[i] Running Linter"
	@$(LINTER) $(INC_FLAGS) src/

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	@echo "[i] Linking: $@"
	@$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[i] Compiling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@echo "[i] Cleaning..."
	@rm -rf $(BUILD_DIR)
	