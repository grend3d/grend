#pragma once
#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>

// XXX: TODO: pass this in from settings
#define TILED_LIGHT_ARRAY 1

struct material {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float roughness;
	float metalness;
	float opacity;
	float alphaCutoff;
};

// light maps
uniform sampler2D diffuse_map;
// TODO: this is the metal-roughness map, need to rename things
//       camelCase while we're at it
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D ambient_occ_map;
// TODO: don't think alpha map is being used anywhere, can remove
uniform sampler2D alpha_map;
uniform sampler2D emissive_map;
uniform sampler2D lightmap;

uniform sampler2D shadowmap_atlas;
uniform sampler2D reflection_atlas;
uniform sampler2D irradiance_atlas;

uniform bool diffuse_vec;
uniform bool emissive_vec;

// TODO: could be in the light info UBO
uniform float renderWidth;
uniform float renderHeight;
uniform float lightThreshold;

uniform float renderID;

uniform vec3 irradiance_probe[6];
uniform vec3 radboxMin;
uniform vec3 radboxMax;
uniform vec3 radprobePosition;

uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

// TODO: UBO for material (except on gles2...)
uniform material anmaterial;
//uniform float time_ms;

struct point_light {
	vec4 position;
	vec4 diffuse;
	float intensity;
	float radius;
	bool casts_shadows;
	float pada; // pad to 16 byte alignment
	vec4 shadowmap[6];
};

struct spot_light {
	vec4 position;
	vec4 diffuse;
	vec4 direction;
	vec4 shadowmap;
	mat4 shadowproj;
	float intensity;
	float radius; // bulb radius
	float angle;
	bool casts_shadows;
};

struct directional_light {
	vec4 position;
	vec4 diffuse;
	vec4 direction;
	vec4 shadowmap;
	mat4 shadowproj;
	float intensity;
	bool casts_shadows;
	float pada[2];
};

// per cluster, tile, whatever, maximum lights that will be evaluated per fragment
#ifndef MAX_LIGHTS
// assume clustered
#if GLSL_VERSION >= 140 /* opengl 3.1+, has uniform buffers, can transfer many more lights */
#define MAX_LIGHTS 28
#else
#define MAX_LIGHTS 8 /* opengl 2.0, need to set each uniform, very limited number of uniforms */
#endif
#endif

// if no light array layout is explicitly defined, default to tiled
#if !defined(CLUSTERED_LIGHT_ARRAY) \
 && !defined(TILED_LIGHT_ARRAY) \
 && !defined(PLAIN_UNIFORM_LIGHT_ARRAY)
#define TILED_LIGHT_ARRAY
#warning "No light array layout defined, defaulting to TILED_LIGHT_ARRAY"
#endif

// gles3, core430+ use SSBOs, clustered lights
// TODO: would be more manageable to split the different uniform layouts
//       into their own includes
#if defined(CLUSTERED_LIGHT_ARRAY) && (GLSL_VERSION == 300 || GLSL_VERSION >= 430)

// for clustered, tiled, number of possible light objects available (ie. in view)
#ifndef MAX_POINT_LIGHT_OBJECTS
#define MAX_POINT_LIGHT_OBJECTS 1024
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS
#define MAX_SPOT_LIGHT_OBJECTS 1024
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS
#define MAX_DIRECTIONAL_LIGHT_OBJECTS 32
#endif

layout(std430, binding = 1) buffer plights {
	uint active_point_lights;
	uint active_spot_lights;
	uint active_directional_lights;

	vec4 reflection_probe[30];
	vec4 refboxMin;
	vec4 refboxMax;
	vec4 refprobePosition;

	point_light point[MAX_POINT_LIGHT_OBJECTS];
	spot_light spot[MAX_SPOT_LIGHT_OBJECTS];
	directional_lights directional[MAX_DIRECTIONAL_LIGHT_OBJECTS];

	uint point_clusters[16*16*16 * MAX_POINT_LIGHT_OBJECTS];
	uint spot_clusters[16*16*16 * MAX_SPOT_LIGHT_OBJECTS];
	// no directional clusters
	//uint directional_clusters[16*16*16 * MAX_DIRECTIONAL_LIGHT_OBJECTS];
};

#define CURRENT_CLUSTER()  (uint(0))

#define ACTIVE_POINTS      (active_point_lights)
#define ACTIVE_SPOTS       (active_spot_lights)
#define ACTIVE_DIRECTIONAL (active_directional_lights)

// TODO: adjust bitmasks for defined sizes
#define POINT_LIGHT(P, CLUSTER)       point[point_clusters[CLUSTER+P] & 0x3ff]
#define SPOT_LIGHT(P, CLUSTER)        spot[spot_clusters[CLUSTER+P] & 0x3ff]
#define DIRECTIONAL_LIGHT(P, CLUSTER) \
	directional[directional_clusters[CLUSTER+P] & 0x1f]

#elif defined(TILED_LIGHT_ARRAY) && GLSL_VERSION >= 140 /* opengl 3.1+, use uniform buffers */

// for clustered, tiled, number of possible light objects available (ie. in view)
#ifndef MAX_POINT_LIGHT_OBJECTS
#define MAX_POINT_LIGHT_OBJECTS 80
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS
#define MAX_SPOT_LIGHT_OBJECTS 24
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS
#define MAX_DIRECTIONAL_LIGHT_OBJECTS 4
#endif

layout (std140, column_major) uniform lights {
	uint uactive_point_lights;
	uint uactive_spot_lights;
	uint uactive_directional_lights;
	uint padding;

	// TODO: configurable number of reflection mips
	// first probe indexes into reflection_atlas, last 4 index into irradiance_atlas
	// (not using reflection_atlas since you can't both read and write to a bound
	//  framebuffer texture)
	vec4 reflection_probe[30];
	vec4 refboxMin;
	vec4 refboxMax;
	vec4 refprobePosition;

	point_light       upoint_lights[MAX_POINT_LIGHT_OBJECTS];
	spot_light        uspot_lights[MAX_SPOT_LIGHT_OBJECTS];
	directional_light udirectional_lights[MAX_DIRECTIONAL_LIGHT_OBJECTS];
};

layout (std140) uniform point_light_tiles {
	// 9*16 grid, other factor of 4 is in uvec4
	// TODO: configurable grid size
	// uvec4 because array elements in std140 must be aligned to
	// 16 byte boundaries, would be very wasteful to have a uint array here...
	uvec4 point_tiles[9*4*(MAX_LIGHTS)];
};

layout (std140) uniform spot_light_tiles {
	// 9*16 grid, other factor of 4 is in uvec4
	uvec4 spot_tiles[9*4*(MAX_LIGHTS)];
};

#define SCREEN_TO_CLUSTER(X, Y) \
	(uint(MAX_LIGHTS/4) * \
		(uint(floor(((Y))*9.0)*16.0) \
		  + uint(floor(((X))*16.0))))

#if defined(FRAGMENT_SHADER)
#define CURRENT_CLUSTER() \
	(SCREEN_TO_CLUSTER(gl_FragCoord.x/renderWidth, gl_FragCoord.y/renderHeight))

#elif defined(VERTEX_SHADER)
uint vertexShaderCluster() {
	vec4 clip = (p*v*m) * vec4(in_Position, 1.0);

	clip.xy /= clip.w;
	clip.xy  = clip.xy*vec2(0.5) + vec2(0.5);

	return SCREEN_TO_CLUSTER(clip.x, clip.y);
}
#define CURRENT_CLUSTER() (vertexShaderCluster())

#else
#error "No implementation of CURRENT_CLUSTER() for this shader type (did you define VERTEX_SHADER/FRAGMENT_SHADER?)"
#endif

#if 1
// XXX: debug macros, avoid locking up the GPU if the buffers aren't zeroed or something
#define ACTIVE_POINTS(CLUSTER) \
	(min(uint(MAX_LIGHTS), point_tiles[CLUSTER][0]))
#define ACTIVE_SPOTS(CLUSTER) \
	(min(uint(MAX_LIGHTS), spot_tiles[CLUSTER][0]))
#define ACTIVE_DIRECTIONAL(CLUSTER) \
	(uactive_directional_lights)

#else
#define ACTIVE_POINTS(CLUSTER) \
	(point_tiles[CLUSTER][0])
#define ACTIVE_SPOTS(CLUSTER) \
	(spot_tiles[CLUSTER][0])
#define ACTIVE_DIRECTIONAL(CLUSTER) \
	(uactive_directional_lights)
#endif

#define POINT_LIGHT_IDX(P, CLUSTER) \
	(point_tiles[CLUSTER + ((P+uint(1))>>uint(2))][(P + uint(1)) & uint(3)])
#define SPOT_LIGHT_IDX(P, CLUSTER) \
	(spot_tiles[CLUSTER + ((P+uint(1))>>uint(2))][(P + uint(1)) & uint(3)])
#define DIRECTIONAL_LIGHT_IDX(P, CLUSTER) \
	(P)

#define POINT_LIGHT(P)       (upoint_lights[P])
#define SPOT_LIGHT(P)        (uspot_lights[P])
#define DIRECTIONAL_LIGHT(P) (udirectional_lights[P])

#define ACTIVE_POINTS_RAW      (uactive_point_lights)
#define ACTIVE_SPOTS_RAW       (uactive_spot_lights)
#define ACTIVE_DIRECTIONAL_RAW (uactive_directional_lights)

// otherwise fallback to regular old uniforms
#elif defined(PLAIN_UNIFORM_LIGHT_ARRAY)

uniform uint active_point_lights;
uniform uint active_spot_lights;
uniform uint active_directional_lights;

// TODO: configurable number of reflection mips

// first probe indexes into reflection_atlas, last 4 index into irradiance_atlas
// (not using reflection_atlas since you can't both read and write to a bound
//  framebuffer texture)
uniform vec4 reflection_probe[30];
uniform vec4 refboxMin;
uniform vec4 refboxMax;
uniform vec4 refprobePosition;

uniform point_light       point_lights[MAX_LIGHTS];
uniform spot_light        spot_lights[MAX_LIGHTS];
uniform directional_light directional_lights[MAX_LIGHTS];

#define CURRENT_CLUSTER()  (uint(0)) 

// for loop iterators, can't use uniform to index lights on gles2
#if GLSL_VERSION < 140
#define ACTIVE_POINTS(CLUSTER)      (GLES2_MAX_POINT_LIGHTS)
#define ACTIVE_SPOTS(CLUSTER)       (GLES2_MAX_SPOT_LIGHTS)
#define ACTIVE_DIRECTIONAL(CLUSTER) (GLES2_MAX_DIRECTIONAL_LIGHTS)

#define ACTIVE_POINTS_RAW      (GLES2_MAX_POINT_LIGHTS)
#define ACTIVE_SPOTS_RAW       (GLES2_MAX_SPOT_LIGHTS)
#define ACTIVE_DIRECTIONAL_RAW (GLES2_MAX_DIRECTIONAL_LIGHTS)

#else
#define ACTIVE_POINTS(CLUSTER)      (active_point_lights)
#define ACTIVE_SPOTS(CLUSTER)       (active_spot_lights)
#define ACTIVE_DIRECTIONAL(CLUSTER) (active_directional_lights)

#define ACTIVE_POINTS_RAW      (active_point_lights)
#define ACTIVE_SPOTS_RAW       (active_spot_lights)
#define ACTIVE_DIRECTIONAL_RAW (active_directional_lights)
#endif

#define POINT_LIGHT_IDX(P, CLUSTER)       (P)
#define SPOT_LIGHT_IDX(P, CLUSTER)        (P)
#define DIRECTIONAL_LIGHT_IDX(P, CLUSTER) (P)

#define POINT_LIGHT(P)       (point_lights[P])
#define SPOT_LIGHT(P)        (spot_lights[P])
#define DIRECTIONAL_LIGHT(P) (directional_lights[P])

#else
#error "No light array layout specified! (see shaders/lib/shading-uniforms.glsl)"
#endif
