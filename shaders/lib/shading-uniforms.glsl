#pragma once

struct point_light {
	vec3 position;
	vec4 diffuse;
	float intensity;
	float radius;
	//samplerCube shadowmap;
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
	//sampler2D shadowmap;
	bool casts_shadows;
	vec3 shadowmap;
};

struct directional_light {
	vec3 position;
	vec4 diffuse;
	vec3 direction;
	float intensity;
	//sampler2D shadowmap;
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

// TODO: "active lights" count, pack lights[] tightly
const int max_lights = 16;
uniform point_light point_lights[max_lights];
uniform spot_light spot_lights[max_lights];
uniform directional_light directional_lights[max_lights];

uniform int active_point_lights;
uniform int active_spot_lights;
uniform int active_directional_lights;

uniform material anmaterial;

uniform float time_ms;

#include <lib/compat.glsl>
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
