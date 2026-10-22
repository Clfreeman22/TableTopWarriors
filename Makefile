.PHONY: all build run test clean

BUILD_DIR = server/build

all: build

build:
	cmake -S server -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/game_server

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	cmake -E rm -rf $(BUILD_DIR) 