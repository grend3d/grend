DIMGUI_SRC = $(wildcard libs/imgui/*.cpp) \
			 libs/imgui/examples/imgui_impl_opengl3.cpp \
			 libs/imgui/examples/imgui_impl_sdl.cpp
DIMGUI_INCLUDES = -I./libs/imgui

SRC  = $(wildcard src/*.cpp)
OBJ  = $(DIMGUI_SRC:.cpp=.o) $(SRC:.cpp=.o)
DEPS = $(SRC:.cpp=.d)


SHADER_SRC = $(wildcard shaders/src/*.vert) $(wildcard shaders/src/*.frag)
SHADER_OUT = $(subst /src/,/out/,$(SHADER_SRC))

INCLUDES = -I./include -I./libs/ $(DIMGUI_INCLUDES)

CXXFLAGS += `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lGL -lGLEW \
            $(INCLUDES) \
            --std=c++17 -Wall -MD \
            -g -Og

all: shaders grend-test
grend-test: $(OBJ)
	$(CXX) $(OBJ) -o $@ $(CXXFLAGS)

-include $(DEPS)

shaders/out/%: shaders/src/%
	@echo SHADER $@
	@cat shaders/version.glsl > $@
	@# simple minifier, remove unneeded spaces
	@cpp -I./shaders -P $< | tr -d '\n' | sed -e 's/  //g' -e 's/ \([+=*\/{}<>:-]*\) /\1/g' -e 's/\([,;]\) /\1/g' >> $@
	@#cpp -I./shaders -P $< >> $@

.PHONY: shaders
shaders: $(SHADER_OUT)

.PHONY: clean-shaders
clean-shaders:
	-rm -f $(SHADER_OUT)

.PHONY: test
test: shaders grend-test
	./grend-test

.PHONY: clean
clean: clean-shaders
	-rm -f grend-test $(OBJ) $(DEPS)
