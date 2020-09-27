include mk/config.mk

BUILD = ./build
SHADER_PATH = shaders/out/
CPPSRC = $(wildcard src/*.cpp)
CSRC   = $(wildcard src/*.c)
OBJ   = $(DIMGUI_SRC:.cpp=.o) $(CPPSRC:.cpp=.o) $(CSRC:.c=.o)
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
# XXX: need to have an include path that has nanovg includes at the base level
DNANOVG_INCLUDES = -I./libs/nanovg/src
DNANOVG_LIB = libs/nanovg/build/libnanovg.a

DSTB_INCLUDES = -I./libs/stb
DSTB_CSRC     = $(wildcard ./libs/stb/*.c)
DSTB_CPPSRC   = $(wildcard ./libs/stb/*.cpp)
DSTB_OBJ      = $(DSTB_CSRC:.c=.o) $(DSTB_CPPSRC:.cpp=.o)

SHADER_SRC = $(wildcard shaders/src/*.vert) $(wildcard shaders/src/*.frag)
SHADER_OUT = $(subst /src/,/out/,$(SHADER_SRC))

INCLUDES = -I./include -I./libs/ \
           $(DIMGUI_INCLUDES) $(DJSON_INCLUDES) $(DNANOVG_INCLUDES) \
		   $(DSTB_INCLUDES)

# only use C flags for emscripten builds
ifeq ($(IS_EMSCRIPTEN),1)
BASEFLAGS += `sdl2-config --cflags`
else
BASEFLAGS += `sdl2-config --cflags --libs`
BASEFLAGS += -lSDL2_ttf -lGL -lGLEW
endif

BASEFLAGS += $(CONF_C_FLAGS) $(CONF_GL_FLAGS) $(CONF_EM_FLAGS) $(DEFINES)
BASEFLAGS += $(INCLUDES) -MD -Wall -MD -O2 -fPIC
BASEFLAGS += -D GR_PREFIX='"$(PREFIX)/share/grend/"'
CXXFLAGS += -std=c++17 $(BASEFLAGS)
CFLAGS   += -std=c11 $(BASEFLAGS)

DEMO_CXXFLAGS := $(CXXFLAGS) $(BUILD)/$(LIBA) $(DNANOVG_LIB)

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
demos: install $(DEMO_TARGETS)

.PHONY: directories
directories: $(BUILD) $(SHADER_PATH)

.PHONY: config-script
config-script: $(BUILD)/grend-config

$(BUILD):
	mkdir -p $@

$(SHADER_PATH):
	mkdir -p $@

$(BUILD)/$(LIBSO): $(OBJ) $(DNANOVG_LIB) $(DSTB_OBJ)
	$(CXX) $(OBJ) $(DNANOVG_LIB) $(DSTB_OBJ) $(CXXFLAGS) -shared -o $@
	-ln -s $(LIBSO) $(BUILD)/libgrend.so

$(BUILD)/$(LIBA): $(OBJ) $(DNANOVG_LIB) $(DSTB_OBJ)
	$(AR) rvs $@ $^

$(DNANOVG_LIB):
	cd libs/nanovg; \
		CC="$(CC)" CXX="$(CXX)" AR="$(AR)" LD="$(CC)" premake5 gmake; \
		cd build; CC="$(CC)" CXX="$(CXX)" AR="$(AR)" LD="$(CC)" make nanovg DEFINES=-DNVG_NO_STB

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
	@echo '        echo "-lgrend -lSDL2_ttf -lGL -lGLEW"' >> $@
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

.PHONY: clean-libs
clean-libs:
	-cd libs/nanovg/build; make clean
	-rm -f $(DSTB_OBJ)

.PHONY: test
test: $(DEMO_TARGETS)
	for thing in $(DEMO_TARGETS); do $$thing; done;

.PHONY: clean
clean: clean-shaders clean-demos clean-libs
	-rm -rf $(BUILD)
	-rm -f $(OBJ) $(DEPS)
