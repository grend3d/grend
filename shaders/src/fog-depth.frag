#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>

IN vec2 f_texcoord;

vec4 fogColor = vec4(vec3(0.4), 1.0);

// TODO: camera uniforms
const float near = 0.1;
const float far = 1000.0;

float linearDepth(float d) {
	return (2.0*far*near)/(far+near - (d*2.0 - 1.0)*(far - near));
}

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;

	vec4 color = texture2D(render_fb, scaled_texcoord);
	float depth = texture2D(render_depth, scaled_texcoord).r;
	if (depth < 1.0) {
		float adj = clamp((linearDepth(depth)) / (far/12.0), 0.0, 1.0);
		FRAG_COLOR = vec4(mix(color.rgb, fogColor.rgb, adj*adj), 1.0);

	} else {
		FRAG_COLOR = vec4(color.rgb, 1.0);
	}
}
