#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>
#include <lib/noise.glsl>

IN vec2 f_texcoord;

uniform int samples;
uniform sampler2DMS colorMS;

void main(void) {
	ivec2 size = textureSize(colorMS);
	ivec2 uv   = ivec2(size*f_texcoord);

	vec4 color = vec4(0);
	vec2 noise = uniformNoise(f_texcoord, time_ms);

	for (int i = 0; i < samples; i++) {
		vec4 samp = texelFetch(colorMS, uv, i);
		color += doTonemap(samp, exposure, noise);
	}

	FRAG_COLOR = color / float(samples);
}
