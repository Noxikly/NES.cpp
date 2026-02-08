CXX ?= g++
INC  := -Iinclude
OPT  := -O2 -flto=auto
WARN := -Wall -Wextra -Wpedantic
CXXFLAGS := -std=c++17 $(INC) $(OPT) $(WARN)

LIBS     := `pkg-config --libs sdl3 luajit`
WIN_LIBS := -static -static-libgcc -static-libstdc++ \
	        -Wl,--export-all-symbols \
	        -lwinmm -lole32 -loleaut32 -luuid -limm32 \
            -lsetupapi -lversion -lgdi32 -luser32 -lkernel32 \
            -lshell32 -ldinput8 -ldxguid -lcfgmgr32


SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp, build/%.o, $(SOURCES))


all: nes

# Сборка
nes: $(OBJECTS)
	$(CXX) $^ -rdynamic -o $@ $(LIBS)

nes_win: $(OBJECTS)
	$(CXX) $^ -o nes $(LIBS) $(WIN_LIBS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build nes