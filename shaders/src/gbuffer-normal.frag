#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>

IN vec2 f_texcoord;

void main(void) {
#if GREND_USE_G_BUFFER
	vec4 color = texture2D(normal_fb, f_texcoord);
	FRAG_COLOR = vec4(color.rgb*0.5 + 0.5, 1.0);

#else
	FRAG_COLOR = vec4(1.0, 0.0, 1.0, 1.0);
#endif
}
