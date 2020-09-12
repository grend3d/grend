#pragma once
#include <lib/shading-uniforms.glsl>

float point_attenuation(int i, vec3 pos) {
	vec3 light_vertex = point_lights[i].position - pos;
	float dist = length(light_vertex);

	// directional/point light is encoded in position.w, constant attenuation
	// for directional light
	return point_lights[i].intensity
		/ pow((dist/point_lights[i].radius) + 1.0, 2.0);
}

float spot_attenuation(int i, vec3 pos) {
	vec3 light_vertex = pos - spot_lights[i].position;
	float dist = length(light_vertex);

	// directional/spot light is encoded in position.w, constant attenuation
	// for directional light
	float lum = spot_lights[i].intensity
		/ pow((dist/spot_lights[i].radius) + 1.0, 2.0);

	float d = dot(normalize(light_vertex), spot_lights[i].direction);
	return (d > spot_lights[i].angle)
		? lum
		: 0.0;
}

float directional_attenuation(int i, vec3 normal) {
	// TODO: something cooler
	return directional_lights[i].intensity/50.0;
}
