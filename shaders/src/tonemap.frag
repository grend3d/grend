#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>

IN vec2 f_texcoord;

uniform vec4 levels;

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;

	vec4 color = texture2D(render_fb, scaled_texcoord);
	FRAG_COLOR = doTonemap(color, exposure);
}
