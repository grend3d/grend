#version 330 core
precision highp float;

in vec3 v_position;
in vec2 v_texcoord;
out vec2 f_texcoord;

void main(void) {
	gl_Position = vec4(v_position, 1.0);
	f_texcoord = v_texcoord;
}
