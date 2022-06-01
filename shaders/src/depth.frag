#define FRAGMENT_SHADER

// version 330 core
precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/davehash.glsl>

void main(void) {
	if (BLEND_MODE_MASKED == 1) {
		vec4  texcolor = texture2D(diffuse_map, f_texcoord);
		float opacity = texcolor.a * anmaterial.opacity;

		if (opacity < anmaterial.alphaCutoff) {
			discard;
		}
	}

	if (BLEND_MODE_DITHERED_BLEND == 1) {
		vec4  texcolor = texture2D(diffuse_map, f_texcoord);
		float opacity = texcolor.a * anmaterial.opacity;
		float noise = hash12(f_texcoord);

		if (opacity < noise) {
			discard;
		}
	}

	// nop, just here for the depth
}
