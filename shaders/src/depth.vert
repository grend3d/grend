#define VERTEX_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>

IN vec3 v_position;
uniform mat4 m, v, p;

void main(void) {
	gl_Position = p*v*m * vec4(v_position, 1.0);
}
