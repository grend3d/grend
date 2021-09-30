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

vec3 reinhard_hdr_inverse(vec3 color, float exposure) {
	vec3 c = color;
	vec3 n = 1.0 / (1.0/pow(c, vec3(2.2)) - 1.0);
	return n / exposure;
}

vec3 reinhard_hdr_modified(vec3 color, float exposure) {
	vec3 c = color * exposure;
	vec3 x = max(vec3(0), c - 0.004);

	return (x*(6.2*x + 0.5)) / (x*(6.2*x + 1.7) + 0.06);
}

vec3 gamma_correct(vec3 color) {
	return pow(color, vec3(1.0/2.2));
}

vec4 doTonemap(in vec4 samp, float exposure, in vec2 noisevec) {
	vec3 dither = vec3(noisevec.x, noisevec.y, 0.5 - noisevec.y) * (2.0 / 256.0);

	//FRAG_COLOR = vec4(reinhard_hdr_modified(color.rgb, exposure), 1.0);
	// TODO: curve editor
	/*
	vec3 mapped =
		reinhard_hdr_modified(samp.rgb, 0.5*exposure) +
		reinhard_hdr_modified(samp.rgb, 0.8*exposure) +
		reinhard_hdr_modified(samp.rgb, 1.2*exposure) +
		reinhard_hdr_modified(samp.rgb, 1.5*exposure);
*/

/*
	vec3 mapped =
		reinhard_hdr(samp.rgb, 0.5*exposure) +
		reinhard_hdr(samp.rgb, 0.8*exposure) +
		reinhard_hdr(samp.rgb, 1.2*exposure) +
		reinhard_hdr(samp.rgb, 1.5*exposure);

	return vec4((mapped / 4.0) + dither, 1.0);
	*/

	return vec4(reinhard_hdr(samp.rgb, exposure), 1.0);
}

vec4 undoTonemap(in vec4 samp, float exposure) {
	//FRAG_COLOR = vec4(reinhard_hdr_modified(color.rgb, exposure), 1.0);
	// TODO: curve editor
	/*
	vec3 mapped =
		reinhard_hdr_modified(samp.rgb, 0.5*exposure) +
		reinhard_hdr_modified(samp.rgb, 0.8*exposure) +
		reinhard_hdr_modified(samp.rgb, 1.2*exposure) +
		reinhard_hdr_modified(samp.rgb, 1.5*exposure);
*/

/*
	vec3 mapped =
		reinhard_hdr_inverse(samp.rgb, 0.5*exposure) +
		reinhard_hdr_inverse(samp.rgb, 0.8*exposure) +
		reinhard_hdr_inverse(samp.rgb, 1.2*exposure) +
		reinhard_hdr_inverse(samp.rgb, 1.5*exposure);
		*/

	return vec4(reinhard_hdr_inverse(samp.rgb, exposure), 1.0);
	//return vec4(mapped / 4.0, 1.0);
}


#if defined(NO_POSTPROCESSING) || defined(SOFT_ANTIALIASING)
#include <lib/noise.glsl>
//#define EARLY_TONEMAP(color, exposure) (reinhard_hdr_modified(color, exposure))
// TODO: UV parameter, not implicit uniform dependency
#define EARLY_TONEMAP(color, exposure, uv) \
	doTonemap(color, exposure, uniformNoise(uv, 0.0))

#else
#define EARLY_TONEMAP(color, exposure, uv) (color)
#endif
