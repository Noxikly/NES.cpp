CXX      := clang++
LDFLAGS  := `pkg-config --libs sdl3 luajit`
INCLUDES := `pkg-config --cflags luajit`
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Wpedantic $(INCLUDES)

SOURCES  := $(wildcard src/*.cpp)
OBJECTS  := $(patsubst src/%.cpp, build/%.o, $(SOURCES))


nes: $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build build/mappers

clean:
	rm -rf build nes
