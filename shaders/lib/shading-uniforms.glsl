#pragma once

struct point_light {
	vec3 position;
	vec4 diffuse;
	float intensity;
	float radius;
	//samplerCube shadowmap;
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
	vec3 shadowmap;
};

struct directional_light {
	vec3 position;
	vec4 diffuse;
	vec3 direction;
	float intensity;
	//sampler2D shadowmap;
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
