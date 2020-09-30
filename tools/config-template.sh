#!/bin/sh
for thing in $@; do
   case $thing in
      --cflags)
        echo "-D GR_PREFIX='\"{{PREFIX}}/share/grend/\"'"
        echo "-I{{PREFIX}}/include"
        echo "-I{{PREFIX}}/include/grend"
        echo "{{CFLAGS}}"
        echo "`sdl2-config --cflags`"
        ;;
      --libs)
        echo "-L{{PREFIX}}/lib"
        echo "-lgrend -lSDL2_ttf -lGL -lGLEW"
        echo "`sdl2-config --libs`"
        ;;
      --help)
        echo "TODO: implement --help"
        exit
   esac
done
