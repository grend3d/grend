#pragma once

struct lightSource {
	vec4 position;
	vec4 diffuse;
	float const_attenuation, linear_attenuation, quadratic_attenuation;
	float specular;
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

uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

// TODO: "active lights" count, pack lights[] tightly
const int max_lights = 32;
uniform lightSource lights[max_lights];
uniform int active_lights;
uniform material anmaterial;
