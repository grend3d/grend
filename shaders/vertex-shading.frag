#version 150
precision highp float;

in vec3 ex_Color;
varying vec2 f_texcoord;
uniform sampler2D mytexture;

// ??? tutorial says to do this, but gl complains about
// gl_FragColor being redefined...
//out vec4 gl_FragColor;

void main(void) {
	vec2 flipped_texcoord = vec2(f_texcoord.x, 1.0 - f_texcoord.y);
	//gl_FragColor = texture2D(mytexture, flipped_texcoord);
	//gl_FragColor = mix(vec4(ex_Color, 1.0), texture2D(mytexture, flipped_texcoord), vec4(0.5));
	gl_FragColor = texture2D(mytexture, flipped_texcoord) * vec4(ex_Color, 1.0) ;
	//gl_FragColor = vec4(ex_Color, 1.0);
	//gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
