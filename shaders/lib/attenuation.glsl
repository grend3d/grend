#pragma once
#include <lib/shading-uniforms.glsl>

float attenuation(int i, vec4 pos) {
	vec3 light_vertex = vec3(lights[i].position - pos);
	float dist = length(light_vertex);

	// directional/point light is encoded in position.w, constant attenuation
	// for directional light
	return mix(1.0, lights[i].intensity / pow((dist/lights[i].radius) + 1, 2.0),
	           lights[i].position.w);
}

