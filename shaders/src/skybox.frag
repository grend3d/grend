#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>

IN vec3 f_texcoord;
uniform samplerCube skytexture;

void main(void) {
	//gl_FragColor = textureCube(skytexture, f_texcoord);
	//gl_FragColor = textureLod(skytexture, f_texcoord, 0);
	FRAG_COLOR = textureLod(skytexture, f_texcoord, 0.0);
}
