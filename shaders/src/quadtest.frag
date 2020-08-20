
#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>

IN vec2 f_texcoord;

void main(void) {
	FRAG_COLOR = vec4(f_texcoord, 0.0, 1.0);

}
