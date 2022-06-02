#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>

void main(void) {
	vec3 albedo = anmaterial.diffuse.rgb * anmaterial.diffuse.w;

	FRAG_COLOR = vec4(albedo, anmaterial.opacity);
#if GREND_USE_G_BUFFER
	FRAG_NORMAL    = TBN[0];
	FRAG_POSITION  = f_position.xyz;
	FRAG_RENDER_ID = renderID;
#endif
}
