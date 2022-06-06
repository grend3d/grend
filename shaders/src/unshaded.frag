#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>
#include <lib/davehash.glsl>

void main(void) {
	vec4  texcolor            = texture2D(diffuse_map, f_texcoord);
	float opacity = texcolor.a * anmaterial.opacity;

	// XXX: assume that the glsl compiler will remove constantly false branches
	//      easy optimization, so should be a safe bet...
	// TODO: do own preprocessing that eliminates these, or better yet,
	//       write my own darn shading language already
	// TODO: move these to macros
	if (BLEND_MODE_MASKED == 1) {
		if (opacity < anmaterial.alphaCutoff) {
			discard;
		}
	}

	if (BLEND_MODE_DITHERED_BLEND == 1) {
		float noise = hash12(gl_FragCoord.xy*1.0);
		if (opacity < noise) {
			discard;
		}
	}

	vec3  emissive_idx        = texture2D(emissive_map, f_texcoord).rgb;
	vec3  metal_roughness_idx = texture2D(specular_map, f_texcoord).rgb;
	float aoidx               = texture2D(ambient_occ_map, f_texcoord).r;
	vec3  normidx             = texture2D(normal_map, f_texcoord).rgb;

	vec3 emissive = anmaterial.emissive.rgb * emissive_idx.rgb;
	vec3 albedo = anmaterial.diffuse.rgb
	            * texcolor.rgb
	            * anmaterial.diffuse.w
	            * f_color.rgb
	            * f_color.w
			    ;

	vec3 normal = normalize(TBN * normalize(normidx * 2.0 - 1.0));
	float metallic  = anmaterial.metalness * metal_roughness_idx.b;
	float roughness = anmaterial.roughness * metal_roughness_idx.g;

	FRAG_COLOR = vec4(albedo, anmaterial.opacity);

	#if GREND_USE_G_BUFFER
		FRAG_NORMAL      = normal;
		FRAG_POSITION    = f_position.xyz;
		FRAG_METAL_ROUGH = vec3(metallic, roughness, 0);
		FRAG_RENDER_ID   = renderID;
	#endif
}
