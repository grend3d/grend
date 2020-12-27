#define VERTEX_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>
#include <lib/instanced-uniforms.glsl>

uniform mat4 m, v, p;

void main(void) {
	f_normal = normalize(v_normal);
	f_texcoord = texcoord;
	f_position = m * transforms[gl_InstanceID] * vec4(in_Position, 1.0);

	gl_Position = p*v*f_position;
}
