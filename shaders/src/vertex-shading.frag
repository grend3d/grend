#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>
#include <lib/shading-uniforms.glsl>

IN vec3 ex_Color;

void main(void) {
	vec3 lightmapped = texture2D(lightmap, f_lightmap).rgb;
	vec4 texcolor = texture2D(diffuse_map, f_texcoord);
	vec3 emissive =
		texture2D(emissive_map, f_texcoord).rgb
		* anmaterial.emissive.rgb;

	float opacity = texcolor.a * anmaterial.opacity;
	vec3 color =
		texcolor.rgb * ex_Color * anmaterial.diffuse.rgb
		+ lightmapped + emissive;

	FRAG_COLOR = vec4(color, opacity);
}
