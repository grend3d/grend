#pragma once

// for settings passed in from cmake
#define OFF 0
#define ON  1

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
		#define FRAG_COLOR    FragColor
		layout (location = 0) out vec4 FragColor;

		#if GREND_USE_G_BUFFER
			layout (location = 1) out vec3  FragNormal;
			layout (location = 2) out vec3  FragPosition;
			layout (location = 3) out vec3  FragMetalRough;
			layout (location = 4) out float FragRenderID;
			// TODO: mesh, render ID

			#define FRAG_NORMAL      FragNormal
			#define FRAG_POSITION    FragPosition
			#define FRAG_METAL_ROUGH FragMetalRough
			#define FRAG_RENDER_ID   FragRenderID
		#endif
	#endif

	#define textureCube(tex, coord) texture(tex, coord)
	#define texture2D(tex, coord) texture(tex, coord)
#endif
