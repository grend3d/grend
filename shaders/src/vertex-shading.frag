#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>
#include <lib/shading-uniforms.glsl>

IN vec3 ex_Color;

void main(void) {
	FRAG_COLOR = texture2D(diffuse_map, f_texcoord) * vec4(ex_Color, 1.0);
}
