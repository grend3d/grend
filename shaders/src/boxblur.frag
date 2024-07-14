#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/constants.glsl>

IN vec2 f_texcoord;

#define SIZE 2

void main(void) {
	vec4 accum = vec4(0.0);
	float count = 0.0;

	for (int y = -SIZE; y <= SIZE; y++) {
		for (int x = -SIZE; x <= SIZE; x++) {
			accum += texture2D(render_fb, f_texcoord + vec2(x, y)/vec2(screen_x, screen_y));
			count += 1.0;
		}
	}

	FRAG_COLOR = vec4(accum / count);
}
