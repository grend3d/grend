#pragma once
#include <lib/compat.glsl>

struct point_light {
	vec3 position;
	vec4 diffuse;
	float intensity;
	float radius;
	bool casts_shadows;
	vec3 shadowmap[6];
};

struct spot_light {
	vec3 position;
	vec4 diffuse;
	vec3 direction;
	float intensity;
	float radius; // bulb radius
	float angle;
	bool casts_shadows;
	vec3 shadowmap;
};

struct directional_light {
	vec3 position;
	vec4 diffuse;
	vec3 direction;
	float intensity;
	bool casts_shadows;
	vec3 shadowmap;
};

struct material {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float roughness;
	float metalness;
	float opacity;
};

// light maps
uniform sampler2D diffuse_map;
// TODO: this is the metal-roughness map, need to rename things
//       camelCase while we're at it
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D ambient_occ_map;
uniform sampler2D alpha_map;
uniform sampler2D emissive_map;

uniform sampler2D shadowmap_atlas;
uniform sampler2D reflection_atlas;
uniform sampler2D irradiance_atlas;

uniform vec3 cameraPosition;
uniform vec3 irradiance_probe[6];
uniform vec3 radboxMin;
uniform vec3 radboxMax;
uniform vec3 radprobePosition;

uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

// per cluster, tile, whatever, maximum lights that will be evaluated per fragment
#ifndef MAX_LIGHTS
#define MAX_LIGHTS 8
#endif

// gles3, core430+ use SSBOs, clustered lights
#if defined(CLUSTERED_LIGHTS) && (GLSL_VERSION == 300 || GLSL_VERSION >= 430)

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

	vec3 reflection_probe[30];
	vec3 refboxMin;
	vec3 refboxMax;
	vec3 refprobePosition;

	point_light point[MAX_POINT_LIGHT_OBJECTS];
	spot_light spot[MAX_SPOT_LIGHT_OBJECTS];
	directional_lights directional[MAX_DIRECTIONAL_LIGHT_OBJECTS];

	uint point_clusters[16*16*16 * MAX_POINT_LIGHT_OBJECTS];
	uint spot_clusters[16*16*16 * MAX_SPOT_LIGHT_OBJECTS];
	// no directional clusters
	//uint directional_clusters[16*16*16 * MAX_DIRECTIONAL_LIGHT_OBJECTS];
};

#define CURRENT_CLUSTER()  (0u)

#define ACTIVE_POINTS      (active_point_lights)
#define ACTIVE_SPOTS       (active_spot_lights)
#define ACTIVE_DIRECTIONAL (active_directional_lights)

// TODO: adjust bitmasks for defined sizes
#define POINT_LIGHT(P, CLUSTER)       point[point_clusters[CLUSTER+P] & 0x3ff]
#define SPOT_LIGHT(P, CLUSTER)        spot[spot_clusters[CLUSTER+P] & 0x3ff]
#define DIRECTIONAL_LIGHT(P, CLUSTER) \
	directional[directional_clusters[CLUSTER+P] & 0x1f]

#elif GLSL_VERSION >= 140 /* opengl 3.1+, use uniform buffers */

// for clustered, tiled, number of possible light objects available (ie. in view)
#ifndef MAX_POINT_LIGHT_OBJECTS
#define MAX_POINT_LIGHT_OBJECTS 64
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS
#define MAX_SPOT_LIGHT_OBJECTS 32
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS
#define MAX_DIRECTIONAL_LIGHT_OBJECTS 4
#endif

layout (std140) uniform lights {
	uint uactive_point_lights;
	uint uactive_spot_lights;
	uint uactive_directional_lights;

	// TODO: configurable number of reflection mips
	// first probe indexes into reflection_atlas, last 4 index into irradiance_atlas
	// (not using reflection_atlas since you can't both read and write to a bound
	//  framebuffer texture)
	vec3 reflection_probe[30];
	vec3 refboxMin;
	vec3 refboxMax;
	vec3 refprobePosition;

	point_light       upoint_lights[MAX_POINT_LIGHT_OBJECTS];
	spot_light        uspot_lights[MAX_SPOT_LIGHT_OBJECTS];
	directional_light udirectional_lights[MAX_DIRECTIONAL_LIGHT_OBJECTS];
};

layout (std140) uniform light_tiles {
	// 8x24 grid, vec4 because array elements in std140 must be aligned to
	// 16 byte boundaries, would be very wasteful to have a uint array here...
	// first entry of each cluster is the number of active lights, hence +1
	vec4 point_tiles[9*4*(MAX_LIGHTS)];
	vec4 spot_tiles[9*4*(MAX_LIGHTS)];
	// TODO: irradiance probe, although that probably only makes sense for
	//       3D clusters...
};

#define CURRENT_CLUSTER() \
	(uint(MAX_LIGHTS/4) * \
		(uint(floor((gl_FragCoord.y / 720.0)*9.0)*16.0) \
		  + uint(floor((gl_FragCoord.x / 1280.0)*16.0))))

#define ACTIVE_POINTS(CLUSTER) \
	(uint(point_tiles[CLUSTER][0]))
#define ACTIVE_SPOTS(CLUSTER) \
	(uint(spot_tiles[CLUSTER][0]))
#define ACTIVE_DIRECTIONAL(CLUSTER) \
	(uactive_directional_lights)

/*
#define POINT_LIGHT(P, CLUSTER) \
	(upoint_lights[P])
#define SPOT_LIGHT(P, CLUSTER) \
	(uspot_lights[P])
#define DIRECTIONAL_LIGHT(P, CLUSTER) \
	udirectional_lights[P]
*/

#define POINT_LIGHT(P, CLUSTER) \
	(upoint_lights[uint(point_tiles[CLUSTER + ((P+1u)>>2u)][(P + 1u) & 3u])])
#define SPOT_LIGHT(P, CLUSTER) \
	(uspot_lights[uint(spot_tiles[CLUSTER + ((P+1u)>>2u)][(P + 1u) & 3u])])
#define DIRECTIONAL_LIGHT(P, CLUSTER) \
	udirectional_lights[P]

#define POINT_LIGHT_RAW(P)       (upoint_lights[P])
#define SPOT_LIGHT_RAW(P)        (uspot_lights[P])
#define DIRECTIONAL_LIGHT_RAW(P) (udirectional_lights[P])

#define ACTIVE_POINTS_RAW      (uactive_point_lights)
#define ACTIVE_SPOTS_RAW       (uactive_spot_lights)
#define ACTIVE_DIRECTIONAL_RAW (uactive_directional_lights)


// otherwise fallback to regular old uniforms
#else

uniform uint active_point_lights;
uniform uint active_spot_lights;
uniform uint active_directional_lights;

// TODO: configurable number of reflection mips

// first probe indexes into reflection_atlas, last 4 index into irradiance_atlas
// (not using reflection_atlas since you can't both read and write to a bound
//  framebuffer texture)
uniform vec3 reflection_probe[30];
uniform vec3 refboxMin;
uniform vec3 refboxMax;
uniform vec3 refprobePosition;

uniform point_light point_lights[MAX_LIGHTS];
uniform spot_light spot_lights[MAX_LIGHTS];
uniform directional_light directional_lights[MAX_LIGHTS];

#define CURRENT_CLUSTER()  (0u) 

// for loop iterators, can't use uniform to index lights on gles2
#if GLSL_VERSION < 140
#define ACTIVE_POINTS      (GLES2_MAX_POINT_LIGHTS)
#define ACTIVE_SPOTS       (GLES2_MAX_SPOT_LIGHTS)
#define ACTIVE_DIRECTIONAL (GLES2_MAX_DIRECTIONAL_LIGHTS)
#else
#define ACTIVE_POINTS      (active_point_lights)
#define ACTIVE_SPOTS       (active_spot_lights)
#define ACTIVE_DIRECTIONAL (active_directional_lights)
#endif

#define POINT_LIGHT(P, CLUSTER)       point_lights[P]
#define SPOT_LIGHT(P, CLUSTER)        spot_lights[P]
#define DIRECTIONAL_LIGHT(P, CLUSTER) directional_lights[P]

#endif

// TODO: UBO for material (except on gles2...)
uniform material anmaterial;
uniform float time_ms;
