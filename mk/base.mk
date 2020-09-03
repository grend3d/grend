BUILD = ./build
SHADER_PATH = shaders/out/
SRC  = $(wildcard src/*.cpp)
OBJ  = $(DIMGUI_SRC:.cpp=.o) $(SRC:.cpp=.o)
LIBSO = libgrend.so.0
LIBA  = libgrend.a
DEPS  = $(SRC:.cpp=.d)

DEMO_TARGETS =
DEMO_CLEAN =

DIMGUI_SRC = $(wildcard libs/imgui/*.cpp) \
			 libs/imgui/examples/imgui_impl_opengl3.cpp \
			 libs/imgui/examples/imgui_impl_sdl.cpp
DIMGUI_INCLUDES = -I./libs/imgui
DJSON_INCLUDES = -I./libs/json/single_include/

SHADER_SRC = $(wildcard shaders/src/*.vert) $(wildcard shaders/src/*.frag)
SHADER_OUT = $(subst /src/,/out/,$(SHADER_SRC))

INCLUDES = -I./include -I./libs/ \
           $(DIMGUI_INCLUDES) $(DJSON_INCLUDES)

CXXFLAGS += `sdl2-config --cflags --libs`
CXXFLAGS += $(INCLUDES)
CXXFLAGS += -lSDL2_image -lSDL2_ttf -lGL -lGLEW
# TODO: remove -g flag
CXXFLAGS += --std=c++17 -Wall -MD -g -Og -fPIC
CXXFLAGS += $(CONF_C_FLAGS) $(CONF_GL_FLAGS)

DEMO_CXXFLAGS := $(CXXFLAGS) $(BUILD)/$(LIBA)

most: directories shaders grend-lib
all: directories shaders grend-lib demos

install: most
	@# TODO: set proper permissions
	mkdir -p "$(SYSROOT)/$(PREFIX)/include"
	mkdir -p "$(SYSROOT)/$(PREFIX)/share/grend/assets"
	mkdir -p "$(SYSROOT)/$(PREFIX)/share/grend/shaders"
	mkdir -p "$(SYSROOT)/$(PREFIX)/lib"
	cp -r include/grend "$(SYSROOT)/$(PREFIX)/include"
	cp -r assets/* "$(SYSROOT)/$(PREFIX)/share/grend/assets"
	cp -r "$(SHADER_PATH)" "$(SYSROOT)/$(PREFIX)/share/grend/shaders"
	install $(BUILD)/$(LIBSO) "$(SYSROOT)/$(PREFIX)/lib/$(LIBSO)"
	-ln -s "$(SYSROOT)/$(PREFIX)/lib/$(LIBSO)" "$(SYSROOT)/$(PREFIX)/lib/libgrend.so"

-include $(DEPS)
-include $(wildcard demos/*/build.mk)

.PHONY: grend-lib
grend-lib: $(BUILD)/$(LIBSO)

.PHONY: shaders
shaders: $(SHADER_OUT)

.PHONY: demos
demos: $(DEMO_TARGETS)

.PHONY: directories
directories: $(BUILD) $(SHADER_PATH)

$(BUILD):
	mkdir -p $(BUILD)

$(SHADER_PATH):
	mkdir -p $(SHADER_PATH)

$(BUILD)/$(LIBSO): $(OBJ)
	$(CXX) $(OBJ) $(CXXFLAGS) -shared -o $@
	-ln -s $(LIBSO) $(BUILD)/libgrend.so

$(BUILD)/$(LIBA): $(OBJ)
	ar rvs $@ $^

# whole build depends on configuration set in Makefile
# should it? maybe not
$(SHADER_OUT) $(OBJ): Makefile
$(DEMO_TARGETS): $(BUILD)/$(LIBA)

shaders/out/%: shaders/src/%
	@echo SHADER $@
	@#cat shaders/version.glsl > $@
	@echo "#version $(CONF_GLSL_STRING)" > $@
	@# simple minifier, remove unneeded spaces
	@#cpp -I./shaders -P $< | tr -d '\n' | sed -e 's/  //g' -e 's/ \([+=*\/{}<>:-]*\) /\1/g' -e 's/\([,;]\) /\1/g' >> $@
	@cpp -I./shaders -P $(CONF_GL_FLAGS) $< >> $@

.PHONY: clean-demos
clean-demos: $(DEMO_CLEAN)

.PHONY: clean-shaders
clean-shaders:
	-rm -f $(SHADER_OUT)

.PHONY: test
test: $(DEMO_TARGETS)
	for thing in $(DEMO_TARGETS); do $$thing; done;

.PHONY: clean
clean: clean-shaders clean-demos
	-rm -rf $(BUILD)
	-rm -f $(OBJ) $(DEPS)
