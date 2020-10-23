#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>
#if SOFT_ANTIALIASING
#include <lib/psaa.glsl>
#endif

IN vec2 f_texcoord;

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;

	float depth = texture2D(render_depth, scaled_texcoord).r;
	vec4 color = texture2D(render_fb, scaled_texcoord);
	vec4 last_color = texture2D(last_frame_fb, f_texcoord);

	//TODO: should tonemapping just be done at the end of shading no matter what?
#ifdef SOFT_ANTIALIASING
	FRAG_COLOR = psaa(scaled_texcoord);
#else
	FRAG_COLOR = vec4(reinhard_hdr_modified(color.rgb, exposure), 1.0);
#endif

}
