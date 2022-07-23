# Grend
An OpenGL game framework/engine

![Demo Screenshot](http://lisp.us.to/data/post7-data/scifi-helmet.png)

## Build instruction

#### Required system libraries:
- SDL2
- Bullet
- GLM
- Glew (for core OpenGL profiles)

After acquiring these fine softwares, you can build the engine as a normal
CMake-based library. 

On linux:

    git clone --recurse-submodules "https://github.com/grend3d/grend"
	mkdir build && cd build
	cmake .. -DCMAKE_INSTALL_PREFIX=<prefix> # or no prefix for global installation
	make && make install

Which you can then link against, see
[the landscape demo](https://github.com/grend3d/landscape-demo)
for an example of how to build against it.

## Supported OS
Linux, Windows, WebGL, and Android

## Features
- Forward renderer with tiled light clustering (basically forward+ without Z prepass)
- glTF and .obj model import, including animations and lights
- Metal-roughness PBR, along with gouraud and (blinn-)phong shading models
- Full HDR rendering pipeline (for core OpenGL profiles)
- A custom ECS layer for game logic
- Bullet physics integration
- Real-time dynamic shadow and reflection pipeline
- Parallax-corrected cubemaps
- Instanced geometry, particles
- Lightweight HRTF approximation for spatial audio
- Robust frustum culling with OBBs
- Dynamic mesh generation with physics integration
- Asyncronous job queue
- Built-in map editor
- Support OpenGL es 2, es 3, core 3.3 and core 4.3
