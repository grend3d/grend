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

#define BROKEN_UINTS
#if defined(BROKEN_UINTS)
#define uint  int
#define uvec2 ivec2
#define uvec3 ivec3
#define uvec4 ivec4
#endif

#if GLSL_VERSION < 130
	#ifdef VERTEX_SHADER
		#define IN attribute
		#define OUT varying
	#endif

	#ifdef FRAGMENT_SHADER
		#define IN varying
		#define OUT /* can't have an output here... */

		#define FRAG_COLOR gl_FragColor
	#endif

	#define textureLod(tex, coord, level) textureCube(tex, coord)
	// used in shadow filtering, default shadow atlas size (right now) is 2048
	#define textureSize(tex, level) (ivec2(2048, 2048))
#endif

#if GLSL_VERSION >= 130
	#define IN in
	#define OUT out

	#ifdef VERTEX_SHADER
	//out vec4 gl_Position; 
	#endif

	#ifdef FRAGMENT_SHADER
		OUT vec4 FragColor;
		#define FRAG_COLOR FragColor
	#endif

	#define textureCube(tex, coord) texture(tex, coord)
	#define texture2D(tex, coord) texture(tex, coord)
#endif
