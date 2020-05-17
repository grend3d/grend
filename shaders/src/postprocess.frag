#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>

in vec2 f_texcoord;

uniform sampler2D render_fb;
uniform sampler2D last_frame_fb;
uniform sampler2D render_depth;

uniform float screen_x;
uniform float screen_y;

uniform float scale_x;
uniform float scale_y;
uniform float exposure;


void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;

	float depth = texture2D(render_depth, scaled_texcoord).r;
	vec4 color = texture2D(render_fb, scaled_texcoord);
	vec4 last_color = texture2D(last_frame_fb, f_texcoord);

	//gl_FragColor = vec4(gamma_correct(color.rgb), 1.0);
	//gl_FragColor = vec4(reinhard_hdr(color.rgb, 1.0), 1.0);
	//gl_FragColor = vec4(reinhard_hdr_modified(color.rgb, exposure), 1.0);
	FRAG_COLOR = vec4(reinhard_hdr_modified(color.rgb, exposure), 1.0);
	//gl_FragColor = vec4(linear_hdr(color.rgb, 2.0), 1.0);
	//gl_FragColor = color;
}
