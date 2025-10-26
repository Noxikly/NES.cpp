CXX      := clang++
CXXFLAGS := -std=c++23 -O2 -Wall -Wextra -Wpedantic `pkg-config --cflags sdl3`
LDFLAGS  := `pkg-config --libs sdl3`

SOURCES  := $(wildcard src/*.cpp) $(wildcard src/mappers/*.cpp)
OBJECTS  := $(patsubst src/%.cpp, build/%.o, $(SOURCES))


nes: $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build build/mappers

clean:
	rm -rf build nes
