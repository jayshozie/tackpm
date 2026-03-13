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
HAS_DEPS := $(shell pkg-config --exists $(DEPS) && echo 'true' || echo 'false')
ifeq ($(HAS_DEPS),false)
$(error "FATAL ERROR: Missing required C libraries: $(DEPS) $(HAS_DEPS)")
endif
DEPS_CFLAGS := $(shell pkg-config --cflags $(DEPS))
DEPS_LDFLAGS := $(shell pkg-config --libs $(DEPS))

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

# Unified Check Target using clang-tidy
check:
	@echo -e "$(BLUE)--- Running Clang-Tidy Check ---$(RESET)"
	@if command -v clang-tidy > /dev/null; then \
		clang-tidy $(SRCS) -- $(CFLAGS) && echo -e "$(GREEN)Pass: Unified check cleared.$(RESET)"; \
	else \
		echo -e "$(RED)clang-tidy not found."; \
	fi

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
