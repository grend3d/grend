#pragma once
#include <lib/utility.glsl>

// fresnel schlick
vec3 F(vec3 f_0, vec3 V, vec3 H) {
	// no not posdot
	return f_0 + (1.0 - f_0) * pow(1.0 - dot(V, H), 5.0);
}
