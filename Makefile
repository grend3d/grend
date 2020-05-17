DIMGUI_SRC = $(wildcard libs/imgui/*.cpp) \
			 libs/imgui/examples/imgui_impl_opengl3.cpp \
			 libs/imgui/examples/imgui_impl_sdl.cpp
DIMGUI_INCLUDES = -I./libs/imgui

SRC  = $(wildcard src/*.cpp)
OBJ  = $(DIMGUI_SRC:.cpp=.o) $(SRC:.cpp=.o)
DEPS = $(SRC:.cpp=.d)

# TODO: small configure script to manage configs
#SHADER_GLSL_STRING="100"
#SHADER_GLSL_VERSION=100
SHADER_GLSL_STRING="300 es"
SHADER_GLSL_VERSION=300

SHADER_SRC = $(wildcard shaders/src/*.vert) $(wildcard shaders/src/*.frag)
SHADER_OUT = $(subst /src/,/out/,$(SHADER_SRC))
#SHADER_FLAGS += -D NO_POSTPROCESSING \
			    -D NO_FORMAT_CONVERSION \
				-D NO_GLPOLYMODE \
				-D NO_ERROR_CHECK \
			    -D GLSL_VERSION=$(SHADER_GLSL_VERSION) \
			    -D GLSL_STRING=\"$(SHADER_GLSL_STRING)\"

SHADER_FLAGS += -D GLSL_VERSION=$(SHADER_GLSL_VERSION) \
			    -D GLSL_STRING=\"$(SHADER_GLSL_STRING)\"

INCLUDES = -I./include -I./libs/ $(DIMGUI_INCLUDES)

CXXFLAGS += `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf -lGL -lGLEW \
            $(INCLUDES) \
            --std=c++17 -Wall -MD \
            -g -Og $(SHADER_FLAGS)
			#-Os $(SHADER_FLAGS)

#TOTAL_MEMORY=67108864
#TOTAL_MEMORY=209715200
#TOTAL_MEMORY=419430400
TOTAL_MEMORY=1048576000

#CXXFLAGS += $(INCLUDES) \
             --std=c++17 -Wall -MD \
			 -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_SDL_IMAGE=2 \
			 -s USE_WEBGL2=1 \
			 -s ERROR_ON_UNDEFINED_SYMBOLS=0 \
			 -s TOTAL_MEMORY=$(TOTAL_MEMORY) \
			 -s OFFSCREEN_FRAMEBUFFER=1 \
			 -O2 $(SHADER_FLAGS)
			 #-Os -g
			 #-s FULL_ES3=1

all: shaders grend-test
grend-test: $(OBJ)
	$(CXX) $(OBJ) -o $@ $(CXXFLAGS)

grend-webgl.html: shaders $(OBJ)
	$(CXX) $(OBJ) -o $@ $(CXXFLAGS) \
		--preload-file assets/tex/cubes/rocky-skyboxes/Skinnarviksberget/ \
		--preload-file assets/obj/low-poly-character-rpg/boy.obj \
		--preload-file assets/obj/low-poly-character-rpg/boy.mtl \
		--preload-file assets/obj/low-poly-character-rpg/textures/ \
		--preload-file assets/obj/tests/DamagedHelmet/glTF \
		--preload-file assets/obj/tests/AnimatedMorphCube/glTF \
		--preload-file assets/obj/tests/donut4.gltf \
		--preload-file assets/obj/tests/test_objects.gltf \
		--preload-file assets/obj/smoothsphere.mtl \
		--preload-file assets/obj/smoothsphere.obj \
		--preload-file assets/obj/smooth-suzanne.obj \
		--preload-file assets/obj/smooth-teapot.obj \
		--preload-file assets/obj/sphere.mtl \
		--preload-file assets/obj/sphere.obj \
		--preload-file assets/obj/suzanne.obj \
		--preload-file assets/obj/teapot.obj \
		--preload-file assets/obj/unitcone.mtl \
		--preload-file assets/obj/unitcone.obj \
		--preload-file assets/tex/white.png \
		--preload-file assets/tex/lightblue-normal.png \
		--preload-file assets/tex/blue.png \
		--preload-file assets/tex/Earthmap720x360_grid.jpg \
		--preload-file assets/tex/iron-rusted4-Unreal-Engine/ \
		--preload-file assets/tex/dims/Textures/Boards.JPG \
		--preload-file assets/tex/dims/Textures/Textures_N/Boards_N.jpg \
		--preload-file shaders/out/ \
		--preload-file save.map 

-include $(DEPS)

shaders/out/%: shaders/src/%
	@echo SHADER $@
	@#cat shaders/version.glsl > $@
	@echo "#version $(SHADER_GLSL_STRING)" > $@
	@# simple minifier, remove unneeded spaces
	@#cpp -I./shaders -P $< | tr -d '\n' | sed -e 's/  //g' -e 's/ \([+=*\/{}<>:-]*\) /\1/g' -e 's/\([,;]\) /\1/g' >> $@
	@cpp -I./shaders -P $(SHADER_FLAGS) $< >> $@

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
