#version 330 core
precision highp float;

in vec2 f_texcoord;
out vec4 FragColor;

uniform sampler2D render_fb;
uniform sampler2D render_depth;

void main(void) {
	float depth = texture2D(render_depth, f_texcoord).r;
	vec4 color = texture2D(render_fb, f_texcoord);

	//FragColor = mix(vec4(1 - color.r, 1 - color.g, 1 - color.b, 1), vec4(1), linear_depth(depth));
	//FragColor = vec4(1);
	FragColor = color;
}
