#define VERTEX_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>
#include <lib/skinning-uniforms.glsl>

uniform mat4 m, v, p;

void main(void) {
	f_normal = normalize(v_normal);
	f_tangent = normalize(v_tangent);
	f_bitangent = normalize(v_bitangent);

	mat4 skinMatrix =
		a_weights.x * joints[int(a_joints.x)]
		+ a_weights.y * joints[int(a_joints.y)]
		+ a_weights.z * joints[int(a_joints.z)]
		+ a_weights.w * joints[int(a_joints.w)];

	vec3 T = normalize(vec3(m*skinMatrix * vec4(v_tangent, 0)));
	vec3 B = normalize(vec3(m*skinMatrix * vec4(v_bitangent, 0)));
	vec3 N = normalize(vec3(m*skinMatrix * vec4(v_normal, 0)));

	TBN = mat3(T, B, N);

	f_texcoord = texcoord;
	f_position = m * vec4(in_Position, 1);

	gl_Position = p*v*m * skinMatrix * vec4(in_Position, 1);
}
