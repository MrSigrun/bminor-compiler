CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS  ?=

BUILD_ROOT := build
OBJ_ROOT   := $(BUILD_ROOT)/obj

ifeq ($(OS),Windows_NT)
    BIN_EXT := .exe
    ENSURE_DIR = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    REMOVE_DIR = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
else
    BIN_EXT :=
    ENSURE_DIR = mkdir -p "$(1)"
    REMOVE_DIR = rm -rf "$(1)"
endif

APP_BIN              := $(BUILD_ROOT)/bminor$(BIN_EXT)
SCANNER_TEST_BIN     := $(BUILD_ROOT)/test_scanner$(BIN_EXT)
PARSER_TEST_BIN      := $(BUILD_ROOT)/test_parser$(BIN_EXT)
HIGHLIGHTER_TEST_BIN := $(BUILD_ROOT)/test_highlighter$(BIN_EXT)
PIPELINE_TEST_BIN    := $(BUILD_ROOT)/test_pipeline$(BIN_EXT)

MODULE_SRCS := $(wildcard src/*/*.cpp)
MODULE_OBJS := $(patsubst src/%.cpp,$(OBJ_ROOT)/%.o,$(MODULE_SRCS))
DRIVER_OBJ  := $(OBJ_ROOT)/main.o

.PHONY: all test clean run

all: $(APP_BIN)

$(APP_BIN): $(DRIVER_OBJ) $(MODULE_OBJS)
	@$(call ENSURE_DIR,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_ROOT)/%.o: src/%.cpp
	@$(call ENSURE_DIR,$(dir $@))
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SCANNER_TEST_BIN): tests/test_scanner.cpp $(MODULE_OBJS)
	@$(call ENSURE_DIR,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(PARSER_TEST_BIN): tests/test_parser.cpp $(MODULE_OBJS)
	@$(call ENSURE_DIR,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(HIGHLIGHTER_TEST_BIN): tests/test_highlighter.cpp $(MODULE_OBJS)
	@$(call ENSURE_DIR,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(PIPELINE_TEST_BIN): tests/test_pipeline.cpp $(MODULE_OBJS)
	@$(call ENSURE_DIR,$(dir $@))
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

test: $(SCANNER_TEST_BIN) $(PARSER_TEST_BIN) $(HIGHLIGHTER_TEST_BIN) $(PIPELINE_TEST_BIN)
	$(SCANNER_TEST_BIN)
	$(PARSER_TEST_BIN)
	$(HIGHLIGHTER_TEST_BIN)
	$(PIPELINE_TEST_BIN)

run: $(APP_BIN)
	$(APP_BIN) --help

clean:
	@$(call REMOVE_DIR,$(BUILD_ROOT))
