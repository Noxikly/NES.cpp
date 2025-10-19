CXX      := clang++
CXXFLAGS := -std=c++23 -O2 -Wall -Wextra -Wpedantic

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp, build/%.o, $(SOURCES))


nes: $(OBJECTS)
	$(CXX) $^ -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p $@

clean:
	rm -rf build nes
