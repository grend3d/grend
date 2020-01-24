SRC  = $(wildcard src/*.cpp)
OBJ  = $(SRC:.cpp=.o)
DEPS = $(SRC:.cpp=.d)

CXXFLAGS += `sdl2-config --cflags --libs` -lSDL2_image -lGL -lGLEW -lglut \
			-I./include --std=c++17 -Wall -g -Og \
			-MD

all: grend-test
grend-test: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@

-include $(DEPS)

.PHONY: test
test: grend-test
	./grend-test

.PHONY: clean
clean:
	-rm -f grend-test $(OBJ) $(DEPS)
