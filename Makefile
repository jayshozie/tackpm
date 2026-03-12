CC := gcc
# Target C11 and use strict Linux-style flags
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes \
          -Wmissing-prototypes -Wold-style-definition -Wformat=2 \
          -Wcast-align -Wpointer-arith -Wbad-function-cast \
          -Iinclude -pthread

DEBUG_FLAGS := -g -O0 -DDEBUG
RELEASE_FLAGS := -O3 -DNDEBUG
LDFLAGS := -pthread

# External Dependencies (Auto-detected via pkg-config)
DEPS := libpq notcurses
DEPS_CFLAGS := $(shell pkg-config --cflags $(DEPS) 2>/dev/null)
DEPS_LDFLAGS := $(shell pkg-config --libs $(DEPS) 2>/dev/null || echo "-lpq -lnotcurses")

CFLAGS += $(DEPS_CFLAGS)
LDFLAGS += $(DEPS_LDFLAGS)

# Directory Structure
SRC_DIR := src
OBJ_DIR := obj
TARGET := tack

# Colors for terminal output
RED := \033[0;31m
GREEN := \033[0;32m
BLUE := \033[0;34m
RESET := \033[0m

# Automatically find all source files and generate object paths
SRCS := $(shell find $(SRC_DIR) -name "*.c")
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all debug release clean check help

# Default target
all: release

# Help target
help:
	@echo -e "$(BLUE)Available targets:$(RESET)"
	@echo -e "  $(GREEN)all$(RESET)     : Default target (builds release)"
	@echo -e "  $(GREEN)debug$(RESET)   : Build with debug symbols and no optimization"
	@echo -e "  $(GREEN)release$(RESET) : Build with optimizations and no debug symbols"
	@echo -e "  $(GREEN)check$(RESET)   : Run static analysis and strict compiler warning checks"
	@echo -e "  $(GREEN)clean$(RESET)   : Remove compiled object files and the target executable"
	@echo -e "  $(GREEN)help$(RESET)    : Show this help message"

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Release build
release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# Enhanced Check Target
check:
	@echo -e "$(BLUE)--- 1. Cppcheck (C11 Static Analysis) ---$(RESET)"
	@if command -v cppcheck > /dev/null; then \
		cppcheck --quiet --enable=all --std=c11 --suppress=missingIncludeSystem --include=include $(SRC_DIR); \
	else \
		echo -e "$(RED)cppcheck not found. Install with 'sudo apt install cppcheck'$(RESET)"; \
	fi
	@echo -e "\n$(BLUE)--- 2. GCC Static Analyzer & Strict Warning Check ---$(RESET)"
	@$(CC) $(CFLAGS) -fanalyzer -Werror -fsyntax-only $(SRCS) && echo -e "$(GREEN)Pass: No warnings found.$(RESET)"

# Link final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile object files and preserve subdirectory structure in OBJ_DIR
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Cleanup: remove obj/ directory and the target executable
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
