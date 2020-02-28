#version 150

in vec3 v_position;
out vec3 f_texcoord;
uniform mat4 p, v;

// unused, here to prevent errors when setting undefined uniforms
uniform mat4 m, v_inv, m_3x3_inv_transp;

void main(void) {
	f_texcoord = v_position;
	gl_Position = p * v * vec4(v_position, 1.0);
}
