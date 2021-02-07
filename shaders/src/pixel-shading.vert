#define VERTEX_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>

uniform mat4 m, v, p;

void main(void) {
	f_normal = normalize(v_normal);
	f_tangent = normalize(v_tangent);
	f_bitangent = normalize(cross(v_tangent, v_normal));

	vec3 T = normalize(vec3(m * vec4(f_tangent, 0)));
	vec3 B = normalize(vec3(m * vec4(f_bitangent, 0)));
	vec3 N = normalize(vec3(m * vec4(f_normal, 0)));

	TBN = mat3(T, B, N);

	f_position = m * vec4(in_Position, 1);
	f_texcoord = texcoord;
	f_lightmap = a_lightmap;
	f_color = v_color;

	gl_Position = p*v * f_position;
}
