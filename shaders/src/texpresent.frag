#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>

IN vec2 f_texcoord;

void main(void) {
	vec4 color = texture2D(render_fb, f_texcoord);

	FRAG_COLOR = vec4(color.rgb, 1.0);
}
