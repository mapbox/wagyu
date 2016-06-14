CXXFLAGS += -I include -std=c++14 -Wall -Wextra -Werror

default: test

build-test: tests/* include/mapbox/geometry/* Makefile
	$(CXX) tests/test.cpp tests/unit/*.cpp $(CXXFLAGS) -I./tests -o test

build-debug: tests/* include/mapbox/geometry/* Makefile
	$(CXX) -g -DDEBUG=1 tests/test.cpp tests/unit/*.cpp $(CXXFLAGS) -I./tests -o test

test: build-test
	./test

debug: build-debug
	./test

clean:
	rm test
