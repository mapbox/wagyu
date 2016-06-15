CC := $(CC)
CXX := $(CXX)
CXXFLAGS := $(CXXFLAGS) -Iinclude -std=c++11
RELEASE_FLAGS := -O3 -DNDEBUG
WARNING_FLAGS := -Wall -Wextra -Werror -Wsign-compare -Wfloat-equal -Wfloat-conversion -Wshadow -Wno-unsequenced
DEBUG_FLAGS := -g -O0 -DDEBUG -fno-inline-functions -fno-omit-frame-pointer

default: test

build-test: tests/* include/mapbox/geometry/* Makefile
	$(CXX) $(RELEASE_FLAGS) tests/test.cpp tests/unit/*.cpp $(WARNING_FLAGS) $(CXXFLAGS) -I./tests -o test

build-debug: tests/* include/mapbox/geometry/* Makefile
	$(CXX) $(DEBUG_FLAGS) tests/test.cpp tests/unit/*.cpp $(WARNING_FLAGS) $(CXXFLAGS) -I./tests -o test

test: build-test
	./test

debug: build-debug
	./test

clean:
	rm test

indent:
	clang-format -i -style="{BasedOnStyle: Google, IndentWidth: 4, UseTab: Never, AllowShortIfStatementsOnASingleLine: false, ColumnLimit: 0, ContinuationIndentWidth: 4, SpaceAfterCStyleCast: true, IndentCaseLabels: true, AllowShortBlocksOnASingleLine: true, AllowShortFunctionsOnASingleLine: true, BreakBeforeBraces: Allman, BinPackParameters: false, ColumnLimit: 80, DerivePointerAlignment: false, PointerAlignment: Middle, SpacesBeforeTrailingComments: 1, BreakBeforeBinaryOperators: None}" $(filter-out ./tests/catch.hpp, $(shell find . '(' -name '*.hpp' -o -name '*.cpp' ')' -type f -print))
