#define VERTEX_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>
#include <lib/billboard-uniforms.glsl>

uniform mat4 m, v, p;
uniform mat4 v_inv;

void main(void) {
	//vec3 pos = positions[gl_InstanceID].xyz * positions[gl_InstanceID].w;
	vec3 pos = positions[gl_InstanceID].xyz;
	float scale = positions[gl_InstanceID].w;

	mat4 transform = mat4(
		vec4(scale, 0, 0, 0),
		vec4(0, scale, 0, 0),
		vec4(0, 0, scale, 0),
		vec4(pos.x, pos.y, pos.z, 1)
	);

	mat4 v_invrot = mat4(
		vec4(v_inv[0].xyz, 0),
		vec4(v_inv[1].xyz, 0),
		vec4(v_inv[2].xyz, 0),
		vec4(0, 0, 0, 1)
	);

	f_normal = mat3(v_invrot)*v_normal;
	f_tangent = v_invrot*v_tangent;
	f_bitangent = vec4(cross(v_normal, v_tangent.xyz) * v_tangent.w, v_tangent.w);

	vec3 T = normalize(vec3(m * f_tangent));
	vec3 B = normalize(vec3(m * f_bitangent));
	vec3 N = normalize(vec3(m * vec4(f_normal, 0)));

	TBN = mat3(T, B, N);
	/*
	mat4 transform = mat4(
		vec4(1, 0, 0, pos.x),
		vec4(0, 1, 0, pos.y),
		vec4(0, 0, 1, pos.z),
		vec4(0, 0, 0, 1)
	);
*/

	f_texcoord = texcoord;
	f_position = transform * v_invrot * m * vec4(in_Position, 1.0);
	f_lightmap = a_lightmap;
	f_color = v_color;

	gl_Position = p*v * f_position;
}
