#pragma once
#include <lib/shading-uniforms.glsl>

float attenuation(int i, vec4 pos) {
	vec3 light_vertex = vec3(lights[i].position - pos);
	float dist = length(light_vertex);

	return 1.0 /
		(lights[i].const_attenuation
		 + lights[i].linear_attenuation * PI * dist
		 + lights[i].quadratic_attenuation * PI * dist * dist);
}

