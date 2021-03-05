#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>
#include <lib/noise.glsl>

IN vec2 f_texcoord;

uniform vec4 levels;

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;
	vec2 noisevec = uniformNoise(f_texcoord, time_ms);

	// displays dithering pattern, leaving in for debugging
	//FRAG_COLOR = vec4(vec3(noisevec, 0.5 - noisevec.x) + 0.5, 1.0);
	//return;

	vec4 color = texture2D(render_fb, scaled_texcoord);
	FRAG_COLOR = doTonemap(color, exposure, noisevec);
	//FRAG_COLOR = vec4((mapped / 4.0), 1.0);
}
