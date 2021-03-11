#pragma once

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>

vec4 decode_r332(int c) {
	return vec4(
		((((c) >> 5) & 7)) / 7.0,
		((((c) >> 2) & 7)) / 7.0,
		((((c))      & 3)) / 3.0,
		// no alpha channel
		1.0
	);
}

ivec4 decode_r332_int(int c) {
	return ivec4(
		(((c >> 5) & 7)),
		(((c >> 2) & 7)),
		(((c)      & 3)),
		1
	);
}

vec4 decode_vec(in sampler2D tex, vec2 uv) {
	// TODO: don't have these two functions on gles2, need fallbacks
	ivec2 size = textureSize(tex, 0);
	vec4 samp = textureLod(tex, uv, 0);

	vec2 pos = (fract(size * uv) - 0.5) * 2.0;
	float rads = 2.0*3.1415926*samp.b;
	float off = (samp.a - 0.5) * 2.0;
	vec2 dir = vec2(cos(rads), sin(rads));
	float dist = dot(dir, pos) + off;
	float color = (dist < 0.0)? samp.r : samp.g;

	return decode_r332(int(color * 255.0));
}

vec4 decode_vec_edges(in sampler2D tex, vec2 uv) {
	// TODO: don't have these two functions on gles2, need fallbacks
	ivec2 size = textureSize(tex, 0);
	vec4 samp = textureLod(tex, uv, 0);

	vec2 pos = (fract(size * uv) - 0.5) * 2.0;
	float rads = 2.0*3.1415926*samp.b;
	float off = (samp.a - 0.5) * 2.0;
	vec2 dir = vec2(cos(rads), sin(rads));
	float dist = dot(dir, pos) + off;
	float color = (dist < 0.0)? samp.r : samp.g;
	float colorb = (dist >= 0.0)? samp.r : samp.g;

	vec4 ret = decode_r332(int(color * 255.0));

	if (samp.r != samp.g) {
		ret.rgb *= 0.5 + (min(abs(dist*0.5), 0.5));
	}

	return ret;
}
