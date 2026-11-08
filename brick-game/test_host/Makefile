# Host build of core/ + games/ with the unit tests. Usage: make -C test_host
CXX ?= g++
CXXFLAGS ?= -std=gnu++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined
SRC := test_main.cpp ../src/core/App.cpp ../src/games/Tetris.cpp

build/tests: $(SRC) $(wildcard ../src/core/*.h ../src/games/*.h ../src/render/Layout.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

.PHONY: test clean
test: build/tests
	./build/tests

clean:
	rm -rf build

.DEFAULT_GOAL := test
