#version 100
precision highp float;

attribute vec3 v_position;
attribute vec2 v_texcoord;
varying vec2 f_texcoord;

void main(void) {
	gl_Position = vec4(v_position, 1.0);
	f_texcoord = v_texcoord;
}
