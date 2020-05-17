#define VERTEX_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>

in vec3 in_Position;
in vec2 texcoord;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;

uniform mat4 m, v, p;

void main(void) {
	f_normal = normalize(v_normal);
	f_tangent = normalize(v_tangent);
	f_bitangent = normalize(v_bitangent);

	vec3 T = normalize(vec3(m * vec4(v_tangent, 0)));
	vec3 B = normalize(vec3(m * vec4(v_bitangent, 0)));
	vec3 N = normalize(vec3(m * vec4(v_normal, 0)));

	TBN = mat3(T, B, N);

	f_texcoord = texcoord;
	f_position = m * vec4(in_Position, 1);

	gl_Position = p*v * f_position;
}
