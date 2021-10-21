#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/davehash.glsl>

IN vec2 f_texcoord;

void main(void) {
	vec4 color = texture2D(render_fb, f_texcoord);
	vec3 noise = hash33(vec3(f_texcoord, time_ms)) * (2.0/256.0);

	FRAG_COLOR = vec4(color.rgb + noise, 1.0);
}
