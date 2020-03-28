#version 100
precision highp float;
precision mediump sampler2D;

varying vec2 f_texcoord;

uniform sampler2D render_fb;
uniform sampler2D render_depth;

void main(void) {
	float depth = texture2D(render_depth, f_texcoord).r;
	vec4 color = texture2D(render_fb, f_texcoord);

	gl_FragColor = color;
}
