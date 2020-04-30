precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

varying vec2 f_texcoord;

uniform sampler2D UItex;

void main(void) {
	vec4 color = texture2D(UItex, f_texcoord);
	gl_FragColor = color;
}
