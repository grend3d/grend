#define VERTEX_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>
#include <lib/billboard-uniforms.glsl>

uniform mat4 m, v, p;

void main(void) {
	vec3 pos = positions[gl_InstanceID].xyz * positions[gl_InstanceID].w;
	mat4 transform = mat4(
		vec4(pos.x, 0, 0, 0),
		vec4(0, pos.y, 0, 0),
		vec4(0, 0, pos.z, 0),
		vec4(0, 0, 0,     1)
	);

	f_normal = normalize(v_normal);
	f_texcoord = texcoord;
	f_position = transform * m * vec4(in_Position, 1.0);

	gl_Position = p*v*f_position;
}
