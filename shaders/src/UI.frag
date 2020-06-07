#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>

IN vec2 f_texcoord;
OUT vec4 gl_FragColor;

uniform sampler2D UItex;

void main(void) {
	vec4 color = texture2D(UItex, f_texcoord);
	//gl_FragColor = color;
	FRAG_COLOR = color;
}
