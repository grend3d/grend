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

