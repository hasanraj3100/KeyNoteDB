CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude
BUILD_DIR := build

CORE_SRCS := src/parser.cpp src/storage.cpp src/command_processor.cpp
CORE_OBJS := $(CORE_SRCS:src/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all test clean

all: $(BUILD_DIR)/kvstore

test: $(BUILD_DIR)/kvstore_tests
	./$(BUILD_DIR)/kvstore_tests

$(BUILD_DIR)/kvstore: src/main.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/kvstore_tests: tests/kvstore_test.cpp $(CORE_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
