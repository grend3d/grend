#version 150

in vec3 f_texcoord;
uniform samplerCube skytexture;

void main(void) {
	gl_FragColor = texture(skytexture, f_texcoord);
}
