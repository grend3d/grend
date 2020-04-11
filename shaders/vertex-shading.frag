#version 100
precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

varying vec3 ex_Color;
varying vec2 f_texcoord;
uniform sampler2D mytexture;

void main(void) {
	gl_FragColor = texture2D(mytexture, f_texcoord) * vec4(ex_Color, 1.0);
}
