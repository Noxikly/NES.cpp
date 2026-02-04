CXX ?= g++
INC  := -Iinclude
OPT  := -O2 -flto=auto
WARN := -Wall -Wextra -Wpedantic
CXXFLAGS := -std=c++17 $(INC) $(OPT) $(WARN)

LIBS     := `pkg-config --libs sdl3 luajit`
WIN_LIBS := -static -static-libgcc -static-libstdc++


SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp, build/%.o, $(SOURCES))


# Тесты
#include tests.mk


all: nes

# Сборка
nes: $(OBJECTS)
	$(CXX) $^ -rdynamic -o $@ $(LIBS)

win: $(OBJECTS)
	$(CXX) $^ -rdynamic -o $@ $(LIBS) $(WIN_LIBS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build nes test_all