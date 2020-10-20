#pragma once

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>

vec3 correctParallax(vec3 p, vec3 cam, vec3 n) {
	vec3 dir = p - cam;
	vec3 ref = reflect(dir, n);

	vec3 intmin = (refboxMin - p) * (1.0 / ref);
	vec3 intmax = (refboxMax - p) * (1.0 / ref);
	vec3 furthest = max(intmin, intmax);
	float dist = min(min(furthest.x, furthest.y), furthest.z);

	vec3 intpos = p + ref*dist;
	vec3 adjdir = intpos - refprobePosition;

	return normalize(adjdir);
}
