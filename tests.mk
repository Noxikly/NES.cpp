TEST_SOURCES := $(wildcard tests/*.cpp)
TEST_OBJECTS := $(patsubst tests/%.cpp, build/%.o, $(TEST_SOURCES))
TEST_LIBS := `pkg-config --libs gtest`


test: $(TEST_OBJECTS) $(filter-out build/main.o, $(OBJECTS))
	$(CXX) $^ -o test_all -rdynamic $(TEST_LIBS) $(LIBS)

build/%.o: tests/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@
