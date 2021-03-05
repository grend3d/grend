#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>

IN vec2 f_texcoord;

uniform vec4 levels;

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;
	vec2 noisevec = fract(time_ms*vec2(
		cos(f_texcoord.x * 55117.0 + f_texcoord.y * 1000211.0 + 1.0),
		sin(f_texcoord.y * 50053.0 + f_texcoord.x * 1000187.0 + 1.1)
	)) - 0.5;

	// displays dithering pattern, leaving in for debugging
	//FRAG_COLOR = vec4(vec3(noisevec, 0.5 - noisevec.x) + 0.5, 1.0);
	//return;

	float depth = texture2D(render_depth, scaled_texcoord).r;
	vec4 color = texture2D(render_fb, scaled_texcoord);
	vec4 last_color = texture2D(last_frame_fb, f_texcoord);

	vec3 dither = vec3(noisevec.x, noisevec.y, 0.5 - noisevec.y) * (2.0 / 256.0);

	//FRAG_COLOR = vec4(reinhard_hdr_modified(color.rgb, exposure), 1.0);
	// TODO: curve editor
	vec3 mapped =
		reinhard_hdr_modified(color.rgb, 0.5*exposure) +
		reinhard_hdr_modified(color.rgb, 0.8*exposure) +
		reinhard_hdr_modified(color.rgb, 1.2*exposure) +
		reinhard_hdr_modified(color.rgb, 1.5*exposure);

	FRAG_COLOR = vec4((mapped / 4.0) + dither, 1.0);
	//FRAG_COLOR = vec4((mapped / 4.0), 1.0);
}
