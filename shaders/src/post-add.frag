#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/constants.glsl>

IN vec2 f_texcoord;

uniform sampler2D chain1;
uniform sampler2D chain2;

void main(void) {
	vec4 a = texture2D(chain1, f_texcoord);
	vec4 b = texture2D(chain2, f_texcoord);

	FRAG_COLOR = vec4(a.rgb + b.rgb, (a.a + b.a) * 0.5);
}
