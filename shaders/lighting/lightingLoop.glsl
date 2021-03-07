#pragma once
#include <lib/shading-uniforms.glsl>
#include <lib/shadows.glsl>

#ifndef LIGHT_FUNCTION
#error "You need to define LIGHT_FUNCTION!"
#endif

// adds to total_light variable, which should be defined externally
#define LIGHT_LOOP(CUR_CLUSTER, POSITION, VIEW, ALBEDO, \
                   NORMAL, METALLIC, ROUGHNESS, AOIDX) \
{ \
	for (uint i = uint(0); i < ACTIVE_POINTS(cluster); i++) { \
		uint idx = POINT_LIGHT_IDX(i, CUR_CLUSTER); \
		float atten = point_attenuation(idx, POSITION); \
		float shadow = point_shadow(idx, POSITION); \
\
		vec3 lum = LIGHT_FUNCTION(POINT_LIGHT(idx).position.xyz, \
		                          POINT_LIGHT(idx).diffuse,  \
		                          POSITION, VIEW, \
		                          ALBEDO, NORMAL, METALLIC, ROUGHNESS); \
\
		total_light += lum*atten*AOIDX*shadow; \
	} \
\
	for (uint i = uint(0); i < ACTIVE_SPOTS(cluster); i++) { \
		uint idx = SPOT_LIGHT_IDX(i, CUR_CLUSTER); \
		float atten = spot_attenuation(idx, vec3(f_position)); \
		float shadow = spot_shadow(idx, vec3(f_position)); \
		vec3 lum = LIGHT_FUNCTION(SPOT_LIGHT(idx).position.xyz, \
		                          SPOT_LIGHT(idx).diffuse, \
		                          POSITION, VIEW, \
		                          ALBEDO, NORMAL, METALLIC, ROUGHNESS); \
\
		total_light += lum*atten*AOIDX*shadow; \
	} \
\
	for (uint i = uint(0); i < ACTIVE_DIRECTIONAL(cluster); i++) { \
		uint idx = DIRECTIONAL_LIGHT_IDX(i, CUR_CLUSTER); \
		float atten = directional_attenuation(idx, vec3(f_position)); \
		vec3 dir = vec3(f_position) - DIRECTIONAL_LIGHT(idx).direction.xyz; \
		vec3 lum = LIGHT_FUNCTION(dir, \
		                          DIRECTIONAL_LIGHT(idx).diffuse, \
		                          POSITION, VIEW, \
		                          ALBEDO, NORMAL, METALLIC, ROUGHNESS); \
\
		total_light += lum*atten*AOIDX; \
	} \
}
