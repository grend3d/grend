#!/bin/sh
# $SYSROOT allows compiling against a cross-compiled install
# from outside the sysroot (useful for emscripten builds)

for thing in $@; do
   case $thing in
      --cflags)
        echo "-D GR_PREFIX='\"{{PREFIX}}/share/grend/\"'"
        echo "-I$SYSROOT/{{PREFIX}}/include"
        echo "-I$SYSROOT/{{PREFIX}}/include/grend"
        echo "{{CFLAGS}}"
        echo "`sdl2-config --cflags`"
        ;;
      --libs)
        echo "-L$SYSROOT/{{PREFIX}}/lib"
        echo "-lgrend -lSDL2_ttf -lGL -lGLEW"
        echo "`sdl2-config --libs`"
        ;;
      --help)
        echo "TODO: implement --help"
        exit
   esac
done
