CXX      := g++
LDFLAGS  := `pkg-config --libs sdl3 luajit` -rdynamic
INCLUDES := `pkg-config --cflags luajit`
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Wpedantic $(INCLUDES)

TARGET:=nes
SOURCES  := $(wildcard src/*.cpp)
OBJECTS  := $(patsubst src/%.cpp, build/%.o, $(SOURCES))


ifeq ($(OS), Windows_NT)
	TARGET=nes.exe
	LDFLAGS+= -static-libgcc -static-libstdc++ -Wl,-Bstatic \
		      -lstdc++ -lpthread -Wl,-Bdynamic
endif


$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build build/mappers

clean:
	rm -rf build $(TARGET)
