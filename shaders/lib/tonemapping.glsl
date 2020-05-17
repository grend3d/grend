#pragma once

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 linear_hdr(vec3 color, float exposure) {
	//return pow(color*exposure, vec3(1.0/2.2));
	return color*exposure;
}

vec3 reinhard_hdr(vec3 color, float exposure) {
	vec3 c = color * exposure;
	return pow(c / (c + vec3(1)), vec3(1.0 / 2.2));
}

vec3 reinhard_hdr_modified(vec3 color, float exposure) {
	vec3 c = color * exposure;
	vec3 x = max(vec3(0), c - 0.004);

	return (x*(6.2*x + 0.5)) / (x*(6.2*x + 1.7) + 0.06);
}

vec3 gamma_correct(vec3 color) {
	return pow(color, vec3(1.0/2.2));
}

#ifdef NO_POSTPROCESSING
#define EARLY_TONEMAP(color, exposure) (reinhard_hdr_modified(color, exposure))
#else
#define EARLY_TONEMAP(color, exposure) color;
#endif
