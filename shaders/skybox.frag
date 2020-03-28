#version 100
precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

varying vec3 f_texcoord;
uniform samplerCube skytexture;

void main(void) {
	gl_FragColor = textureCube(skytexture, f_texcoord);
}
