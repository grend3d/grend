include mk/config.mk

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
CXXFLAGS += -lSDL2_ttf -lGL -lGLEW
CXXFLAGS += --std=c++17
CXXFLAGS += $(CONF_C_FLAGS) $(CONF_GL_FLAGS)
CXXFLAGS += $(INCLUDES) -MD -Wall -MD -O2 -fPIC
CXXFLAGS += -D GR_PREFIX='"$(PREFIX)/share/grend/"'

DEMO_CXXFLAGS := $(CXXFLAGS) $(BUILD)/$(LIBA)

most: directories shaders grend-lib config-script
all: most demos

install: most
	@# TODO: set proper permissions
	mkdir -p "$(SYSROOT)/$(PREFIX)/include/mk"
	mkdir -p "$(SYSROOT)/$(PREFIX)/share/grend/assets"
	mkdir -p "$(SYSROOT)/$(PREFIX)/share/grend/shaders"
	mkdir -p "$(SYSROOT)/$(PREFIX)/lib"
	mkdir -p "$(SYSROOT)/$(PREFIX)/bin"
	cp -r include/grend "$(SYSROOT)/$(PREFIX)/include"
	cp -r libs/tinygltf "$(SYSROOT)/$(PREFIX)/include/grend"
	cp -r libs/imgui "$(SYSROOT)/$(PREFIX)/include/grend"
	cp -r libs/json/single_include/nlohmann "$(SYSROOT)/$(PREFIX)/include/grend"
	cp -r assets/* "$(SYSROOT)/$(PREFIX)/share/grend/assets"
	cp -r mk "$(SYSROOT)/$(PREFIX)/include/grend"
	cp -r "$(SHADER_PATH)" "$(SYSROOT)/$(PREFIX)/share/grend/shaders"
	install $(BUILD)/$(LIBSO) "$(SYSROOT)/$(PREFIX)/lib/$(LIBSO)"
	install $(BUILD)/$(LIBA) "$(SYSROOT)/$(PREFIX)/lib/$(LIBA)"
	install $(BUILD)/grend-config "$(SYSROOT)/$(PREFIX)/bin/grend-config"
	-ln -s "$(SYSROOT)/$(PREFIX)/lib/$(LIBSO)" "$(SYSROOT)/$(PREFIX)/lib/libgrend.so"

-include $(DEPS)
-include $(wildcard demos/*/build.mk)

.PHONY: grend-lib
grend-lib: $(BUILD)/$(LIBSO) $(BUILD)/$(LIBA)

.PHONY: shaders
shaders: $(SHADER_OUT)

.PHONY: demos
demos: $(DEMO_TARGETS)

.PHONY: directories
directories: $(BUILD) $(SHADER_PATH)

.PHONY: config-script
config-script: $(BUILD)/grend-config

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

$(BUILD)/grend-config: Makefile
	@echo '#!/bin/sh' > $@
	@echo 'for thing in $$@; do' >> $@
	@echo '   case $$thing in' >> $@
	@echo '      --cflags)' >> $@
	@echo '        echo "-I$(PREFIX)/include"' >> $@
	@echo '        echo "-I$(PREFIX)/include/grend"' >> $@
	@echo "        echo \"$(CONF_GL_FLAGS)\"" >> $@
	@echo '        echo "`sdl2-config --cflags`"' >> $@
	@echo '        ;;' >> $@
	@echo '      --libs)' >> $@
	@echo '        echo "-L$(PREFIX)/lib"' >> $@
	@echo '        echo "-lgrend -lSDL2_ttf -LGL -lGLEW"' >> $@
	@echo '        echo "`sdl2-config --libs`"' >> $@
	@echo '        ;;' >> $@
	@echo '      --help)' >> $@
	@echo '        echo "TODO: implement --help"' >> $@
	@echo '        exit' >> $@
	@echo '   esac' >> $@
	@echo 'done' >> $@
	@chmod +x $@

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
