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
	float roughness;
	float metalness;
	float opacity;
};

// light maps
uniform sampler2D diffuse_map;
// TODO: this is the metal-roughness map, need to rename things
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D ambient_occ_map;
uniform sampler2D alpha_map;
uniform samplerCube skytexture;

uniform sampler2D shadowmap_atlas;
uniform sampler2D reflection_atlas;
uniform vec3 reflection_probe[6];

uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

// per cluster, tile, whatever, maximum lights that will be evaluated per fragment
#ifndef MAX_LIGHTS
#define MAX_LIGHTS 8
#endif

#ifndef MAX_LIGHT_OBJECTS
// for clustered, tiled, number of possible light objects available
#define MAX_LIGHT_OBJECTS 1024
#endif

// gles3, core430+ use SSBOs, clustered lights
#if GLSL_VERSION == 300 || GLSL_VERSION >= 430
layout(std430, binding = 1) buffer plights {
	uint point_lights;
	uint spot_lights;
	uint directional_lights;

	point_light point[1024];
	uint point_clusters[16*16*16 * MAX_LIGHTS];

	spot_light spot[1024];
	uint spot_clusters[16*16*16 * MAX_LIGHTS];

	directional_lights directional[1024];
	uint spot_clusters[16*16*16 * MAX_LIGHTS];
};

// otherwise fallback to regular old uniforms (TODO: something fancier, tiling?)
#else

uniform point_light point_lights[MAX_LIGHTS];
uniform spot_light spot_lights[MAX_LIGHTS];
uniform directional_light directional_lights[MAX_LIGHTS];

uniform int active_point_lights;
uniform int active_spot_lights;
uniform int active_directional_lights;
#endif

// TODO: UBO for material (except on gles2...)
uniform material anmaterial;
uniform float time_ms;

// for loop iterators, can't use uniform to index lights on gles2
#if GLSL_VERSION < 300
#define ACTIVE_POINTS      (GLES2_MAX_POINT_LIGHTS)
#define ACTIVE_SPOTS       (GLES2_MAX_SPOT_LIGHTS)
#define ACTIVE_DIRECTIONAL (GLES2_MAX_DIRECTIONAL_LIGHTS)
#else
#define ACTIVE_POINTS      (active_point_lights)
#define ACTIVE_SPOTS       (active_spot_lights)
#define ACTIVE_DIRECTIONAL (active_directional_lights)
#endif
