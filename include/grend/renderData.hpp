#pragma once
#include <grend/openglIncludes.hpp>

// header defining GPU buffer data structures

// per cluster, tile, whatever, maximum lights that will be evaluated per fragment
#ifndef MAX_LIGHTS
// assume clustered
#if GLSL_VERSION >= 140 /* opengl 3.1+, has uniform buffers, can transfer many more lights */
#define MAX_LIGHTS 28
#else
#define MAX_LIGHTS 8 /* opengl 2.0, need to set each uniform, very limited number of uniforms */
#endif
#endif

// clustered light array definitions
#ifndef MAX_POINT_LIGHT_OBJECTS_CLUSTERED
#define MAX_POINT_LIGHT_OBJECTS_CLUSTERED 1024
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS_CLUSTERED
#define MAX_SPOT_LIGHT_OBJECTS_CLUSTERED 1024
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS_CLUSTERED
#define MAX_DIRECTIONAL_LIGHT_OBJECTS_CLUSTERED 32
#endif

// tiled light array definitions
// XXX: so, there's a ridiculous number of bugs in the gles3 implementation 
//      of the adreno 3xx-based phone I'm testing on, one of which is that
//      multiple UBOs aren't actually usable... so, need a fallback to
//      the old style non-tiled lights array, but using one UBO
//
//      https://developer.qualcomm.com/forum/qdn-forums/software/adreno-gpu-sdk/29611
#if defined(USE_SINGLE_UBO)
#ifndef MAX_POINT_LIGHT_OBJECTS_TILED
#define MAX_POINT_LIGHT_OBJECTS_TILED MAX_LIGHTS
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS_TILED
#define MAX_SPOT_LIGHT_OBJECTS_TILED MAX_LIGHTS
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED
#define MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED 4
#endif

#else /* not defined(USE_SINGLE_UBO) */
// otherwise, tiled lighting, lots more lights available
#ifndef MAX_POINT_LIGHT_OBJECTS_TILED
#define MAX_POINT_LIGHT_OBJECTS_TILED 80
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS_TILED
#define MAX_SPOT_LIGHT_OBJECTS_TILED 24
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED
#define MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED 4
#endif
#endif

namespace grendx {

struct point_std140 {
	GLfloat position[4];   // 0
	GLfloat diffuse[4];    // 16
	GLfloat intensity;     // 32
	GLfloat radius;        // 36
	GLuint  casts_shadows; // 40
	GLfloat padding;       // 44, pad to 48
	GLfloat shadowmap[24]; // 48, end 144
} __attribute__((packed));

struct spot_std140 {
	GLfloat position[4];    // 0
	GLfloat diffuse[4];     // 16
	GLfloat direction[4];   // 32
	GLfloat shadowmap[4];   // 48
	GLfloat shadowproj[16]; // 64
	GLfloat intensity;      // 128
	GLfloat radius;         // 132
	GLfloat angle;          // 136
	GLuint  casts_shadows;  // 140, end 144
} __attribute__((packed));

struct directional_std140 {
	GLfloat position[4];     // 0
	GLfloat diffuse[4];      // 16
	GLfloat direction[4];    // 32
	GLfloat shadowmap[4];    // 48
	GLfloat shadowproj[16];  // 64
	GLfloat intensity;       // 128
	GLuint  casts_shadows;   // 132
	GLfloat padding[2];      // 136, pad to 144
} __attribute__((packed));

struct lights_std140 {
	// TODO: move reflection probes to buffer list
	GLfloat reflection_probe[30 * 4];
	GLfloat refboxMin[4];
	GLfloat refboxMax[4];
	GLfloat refprobePosition[4];       // end 528 + 16
} __attribute__((packed));

// TODO: might be useful to have this as a templated type
struct point_light_buffer_std140 {
	GLuint uactive_point_lights;       // 0
	GLuint padding[3];                 // pad to 16
	point_std140 upoint_lights[MAX_POINT_LIGHT_OBJECTS_TILED];
} __attribute__((packed));

struct spot_light_buffer_std140 {
	GLuint uactive_spot_lights;       // 0
	GLuint padding[3];                // pad to 16
	spot_std140 uspot_lights[MAX_SPOT_LIGHT_OBJECTS_TILED];
} __attribute__((packed));

struct directional_light_buffer_std140 {
	GLuint uactive_directional_lights; // 0
	GLuint padding[3];                 // pad to 16
	directional_std140 udirectional_lights[MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED];
} __attribute__((packed));

// check to make sure everything will fit in the (specifications) minimum required UBO
// TODO: configurable maximum, or adjust size for the largest UBO on the platform
static_assert(sizeof(lights_std140) <= 16384,
              "Packed light attributes in lights_std140 must be <=16384 bytes!");

struct light_tiles_std140 {
	GLuint indexes[9*16*MAX_LIGHTS];
} __attribute__((packed));

static_assert(sizeof(light_tiles_std140) <= 16384,
              "Packed light tiles in light_tiles_std140 must be <=16384 bytes!");


// namespace grendx
}
