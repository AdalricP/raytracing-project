CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -g $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)
SRC_DIR = src
BUILD_DIR = build
OUTPUT_DIR = output
TARGET = $(BUILD_DIR)/raytracer
OUTPUT = $(OUTPUT_DIR)/image.ppm

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

all: $(TARGET)

# Explicit build target alias for convenience
build: $(TARGET)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

run: $(TARGET) | $(OUTPUT_DIR)
	./$(TARGET) > $(OUTPUT)
	@echo "Image rendered to $(OUTPUT)"

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR)

# Play target: clean, build, then run
play: clean build run

.PHONY: all build run clean play
