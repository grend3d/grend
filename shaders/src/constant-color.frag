// similar to unshaded, except this doesn't use material info,
// color is set in the 'outputColor' uniform
#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>
#include <lib/shading-uniforms.glsl>

uniform vec4 lols;
uniform vec4 outputColor;

void main(void) {
	FRAG_COLOR = outputColor;

	#if GREND_USE_G_BUFFER
		vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
		vec3 normal = normalize(TBN * normalize(normidx * 2.0 - 1.0));

		FRAG_NORMAL      = normal;
		FRAG_POSITION    = f_position.xyz;
		FRAG_METAL_ROUGH = vec3(0, 1, 0);
		FRAG_RENDER_ID   = renderID;
	#endif
}
