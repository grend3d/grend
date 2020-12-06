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
}
