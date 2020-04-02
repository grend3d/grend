SRC  = $(wildcard src/*.cpp)
OBJ  = $(SRC:.cpp=.o)
DEPS = $(SRC:.cpp=.d)

INCLUDES = -I./include -I./libs/

CXXFLAGS += `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lGL -lGLEW \
            $(INCLUDES) \
            --std=c++17 -Wall -MD \
            -g -Og

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
