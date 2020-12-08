#pragma once
#include <lib/shading-uniforms.glsl>

float point_attenuation(uint i, uint cluster, vec3 pos) {
	vec3 light_vertex = POINT_LIGHT(i, cluster).position - pos;
	float dist = length(light_vertex);
	float falloff = pow((dist/POINT_LIGHT(i, cluster).radius) + 1.0, 2.0);

	// directional/point light is encoded in position.w, constant attenuation
	// for directional light
	return max(0.0, POINT_LIGHT(i, cluster).intensity
		/ (falloff - lightThreshold) - lightThreshold);
}

float spot_attenuation(uint i, uint cluster, vec3 pos) {
	vec3 light_vertex = pos - SPOT_LIGHT(i, cluster).position;
	float dist = length(light_vertex);
	float falloff = pow((dist/SPOT_LIGHT(i, cluster).radius) + 1.0, 2.0);

	// directional/spot light is encoded in position.w, constant attenuation
	// for directional light
	float lum = max(0.0, SPOT_LIGHT(i, cluster).intensity
		/ (falloff - lightThreshold) - lightThreshold);

	float d = dot(normalize(light_vertex), SPOT_LIGHT(i, cluster).direction);
	return (d > SPOT_LIGHT(i, cluster).angle)
		? lum
		: 0.0;
}

float directional_attenuation(uint i, uint cluster, vec3 normal) {
	// TODO: something cooler
	return DIRECTIONAL_LIGHT(i, cluster).intensity/50.0;
}
