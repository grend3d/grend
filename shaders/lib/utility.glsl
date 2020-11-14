#pragma once

float nzdot(vec3 a, vec3 b) {
	float d = dot(a, b);

	return (d < 0.0)? min(-0.001, d) : max(0.001, d);
}

float posdot(vec3 a, vec3 b) {
	return max(0.001, dot(a, b));
}

float mindot(vec3 a, vec3 b) {
	return max(0.0, dot(a, b));
}

float absdot(vec3 a, vec3 b) {
	return abs(dot(a, b));
}
