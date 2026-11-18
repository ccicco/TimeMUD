# TimeMUD Makefile - round 0
# Lives in code/ (Diku precedent: src/Makefile). Objects and binary go to the
# root build/ dir, which is generated and gitignored.
#
# Usage:  make -C code        (from repo root)
#         make -C code run    (build + smoke-run)

CXX      := clang++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -g
TOP      := $(abspath $(dir $(abspath $(lastword $(MAKEFILE_LIST))))/..)
BUILD    := $(TOP)/build
BIN      := $(BUILD)/timemud

# Every category's sources, mirrored into build/
SRC := $(sort $(wildcard */*.cpp))
OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(SRC))
HDR := $(wildcard include/*.h)

.PHONY: all run clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# build/<category>/<file>.o, with include/ on the search path
$(BUILD)/%.o: %.cpp $(HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

run: all
	cd $(TOP) && ./build/timemud

clean:
	rm -rf $(BUILD)
