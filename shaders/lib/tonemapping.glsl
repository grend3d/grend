#pragma once

vec3 gamma_correct(vec3 color) {
	return pow(color, vec3(1.0/2.2));
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 linear_hdr(vec3 color, float exposure) {
	return color*exposure;
}

vec3 reinhard_hdr(vec3 color, float exposure) {
	vec3 c = color * exposure;
	return pow(c / (c + vec3(1)), vec3(1.0 / 2.2));
}

// nicer reinhard variant (imo), avoids muting specular highlights
vec3 reinhard_hdr_modified(vec3 color, float exposure) {
	vec3 c = color * exposure;
	return pow(c / (c + vec3(0.4)), vec3(1.0 / 1.7));
}

vec3 reinhard_hdr_inverse(vec3 color, float exposure) {
	vec3 c = color;
	vec3 n = 1.0 / (1.0/pow(c, vec3(2.2)) - 1.0);
	return n / exposure;
}

vec3 reinhard_modified_inverse(vec3 color, float exposure) {
	vec3 c = color;
	vec3 n = 1.0 / (1.0/pow(c, vec3(1.7)) - 0.4);
	return n / exposure;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
// TODO: inverse
vec3 ACESFilm(vec3 color, float exposure) {
	vec3 x = color*exposure;

    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return clamp((x*(a*x+b))/(x*(c*x+d)+e), vec3(0.0), vec3(1.0));
}

// TODO: no need for this anymore really
vec4 doTonemap(in vec4 samp, float exposure) {
	//return vec4(reinhard_hdr_modified(samp.rgb, exposure), 1.0);
	//return vec4(reinhard_hdr(samp.rgb, exposure), 1.0);
	return vec4(gamma_correct(ACESFilm(samp.rgb, exposure)), 1.0);
}

vec4 undoTonemap(in vec4 samp, float exposure) {
	// TODO: inverse of ACES
	return vec4(reinhard_hdr_inverse(samp.rgb, exposure), 1.0);
}

#if defined(NO_POSTPROCESSING) || defined(SOFT_ANTIALIASING)
#include <lib/noise.glsl>
//#define EARLY_TONEMAP(color, exposure) (reinhard_hdr_modified(color, exposure))
// TODO: UV parameter, not implicit uniform dependency
#define EARLY_TONEMAP(color, exposure, uv) \
	doTonemap(color, exposure)

#else
#define EARLY_TONEMAP(color, exposure, uv) (color)
#endif
