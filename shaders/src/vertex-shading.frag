#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>

in vec3 ex_Color;
in vec2 f_texcoord;
uniform sampler2D mytexture;

void main(void) {
	gl_FragColor = texture2D(mytexture, f_texcoord) * vec4(ex_Color, 1.0);
}
