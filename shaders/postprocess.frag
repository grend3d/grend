#version 100
precision highp float;
precision mediump sampler2D;

varying vec2 f_texcoord;

uniform sampler2D render_fb;
uniform sampler2D last_frame_fb;
uniform sampler2D render_depth;

uniform float screen_x;
uniform float screen_y;

uniform float scale_x;
uniform float scale_y;


// http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 linear_hdr(vec3 color, float exposure) {
	//return pow(color*exposure, vec3(1.0/2.2));
	return color*exposure;
}

vec3 reinhard_hdr(vec3 color, float exposure) {
	vec3 c = color * exposure;
	return pow(c / (c + vec3(1)), vec3(1.0 / 2.2));
}

vec3 reinhard_hdr_modified(vec3 color, float exposure) {
	vec3 c = color * exposure;
	vec3 x = max(vec3(0), c - 0.004);

	return (x*(6.2*x + 0.5)) / (x*(6.2*x + 1.7) + 0.06);
}

vec3 gamma_correct(vec3 color) {
	return pow(color, vec3(1.0/2.2));
}

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;

	float depth = texture2D(render_depth, scaled_texcoord).r;
	vec4 color = texture2D(render_fb, scaled_texcoord);
	vec4 last_color = texture2D(last_frame_fb, f_texcoord);

	//gl_FragColor = vec4(gamma_correct(color.rgb), 1.0);
	//gl_FragColor = vec4(reinhard_hdr(color.rgb, 1.0), 1.0);
	gl_FragColor = vec4(reinhard_hdr_modified(color.rgb, 1.0), 1.0);
	//gl_FragColor = vec4(linear_hdr(color.rgb, 2.0), 1.0);
	//gl_FragColor = color;
}
