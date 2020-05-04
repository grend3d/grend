precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

attribute vec3 v_position;
varying vec3 f_texcoord;
uniform mat4 p, v;

// unused, here to prevent errors when setting undefined uniforms
uniform mat4 m, v_inv, m_3x3_inv_transp;

void main(void) {
	f_texcoord = v_position;
	vec4 outpos = p * v * vec4(v_position, 1.0);
	gl_Position = outpos.xyww;
}
