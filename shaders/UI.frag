#version 330 core
precision highp float;

in vec2 f_texcoord;
out vec4 FragColor;

uniform sampler2D UItex;

void main(void) {
	vec4 color = texture2D(UItex, f_texcoord);
	FragColor = color;
}
