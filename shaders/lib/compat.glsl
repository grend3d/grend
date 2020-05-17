#pragma once

#ifndef VERTEX_SHADER
#ifndef FRAGMENT_SHADER
#error You need to define the shader type! (VERTEX_SHADER or FRAGMENT_SHADER)
#endif
#endif

#ifdef VERTEX_SHADER
#ifdef FRAGMENT_SHADER
#error You've said too much! can't be both VERTEX_SHADER and FRAGMENT_SHADER
#endif
#endif

#ifndef GLSL_VERSION
#define GLSL_VERSION 300
#endif

#if GLSL_VERSION < 130

#ifdef VERTEX_SHADER
#define in attribute
#define out varying
#endif

#ifdef FRAGMENT_SHADER
#define in varying
#define out /* can't have an output here... */

#define FRAG_COLOR gl_FragColor
#endif

#define textureLod(tex, coord, level) textureCube(tex, coord)

#endif

#if GLSL_VERSION >= 130
#ifdef FRAGMENT_SHADER
out vec4 FragColor;
#define FRAG_COLOR FragColor
#endif

#ifdef VERTEX_SHADER
//out vec4 gl_Position; 
#endif

#define textureCube(tex, coord) texture(tex, coord)
#define texture2D(tex, coord) texture(tex, coord)

#endif
