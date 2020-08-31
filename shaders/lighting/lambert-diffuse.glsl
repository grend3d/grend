#pragma once
#include <lib/utility.glsl>

float lambert_diffuse(float rough, vec3 L, vec3 N, vec3 V) {
	return max(0.0, mindot(N, L));
}
