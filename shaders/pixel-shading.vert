#version 150

attribute vec3 in_Position;
attribute vec2 texcoord;
attribute vec3 v_normal;
attribute vec3 v_tangent;
attribute vec3 v_bitangent;

// exports to the fragment shader
varying vec4 f_position;
varying vec3 f_normal;
varying vec3 f_tangent;
varying vec3 f_bitangent;
varying vec2 f_texcoord;

varying mat3 TBN;

uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

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
