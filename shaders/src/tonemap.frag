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

	float depth = texture2D(render_depth, scaled_texcoord).r;
	vec4 color = texture2D(render_fb, scaled_texcoord);
	vec4 last_color = texture2D(last_frame_fb, f_texcoord);

	//FRAG_COLOR = vec4(reinhard_hdr_modified(color.rgb, exposure), 1.0);
	// TODO: curve editor
	vec3 mapped =
		reinhard_hdr_modified(color.rgb, 0.5*exposure) +
		reinhard_hdr_modified(color.rgb, 0.8*exposure) +
		reinhard_hdr_modified(color.rgb, 1.2*exposure) +
		reinhard_hdr_modified(color.rgb, 1.5*exposure);

	FRAG_COLOR = vec4(mapped / 4.0, 1.0);
}
