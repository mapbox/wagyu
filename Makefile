CXXFLAGS += -I include --std=c++14 -Wall -Wextra -Werror
MASON ?= .mason/mason

default: test

build-test: tests/* include/mapbox/geometry/* Makefile
	$(CXX) tests/test.cpp tests/unit/*.cpp $(CXXFLAGS) -I./tests -o test

test: build-test
	./test

clean:
	rm test
