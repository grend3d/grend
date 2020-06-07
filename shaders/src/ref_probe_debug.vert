#define VERTEX_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>

uniform mat4 m, v, p;

void main(void) {
	f_normal = normalize(v_normal);
	f_texcoord = texcoord;
	f_position = m * vec4(in_Position, 1);

	gl_Position = p*v*m * vec4(in_Position, 1);
}
