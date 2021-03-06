#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1
#define ENABLE_REFRACTION 1

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>
#include <lib/tonemapping.glsl>
#include <lighting/metal-roughness-pbr.glsl>
#include <lib/shadows.glsl>
#include <lib/constants.glsl>

void main(void) {
	uint cluster = CURRENT_CLUSTER();
	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	mat4 mvp = p*v*m;

	vec3 metal_roughness_idx =
		anmaterial.metalness*texture2D(specular_map, f_texcoord).rgb;
	vec4 texcolor = texture2D(diffuse_map, f_texcoord);
	vec3 albedo = anmaterial.diffuse.rgb * texcolor.rgb * anmaterial.diffuse.w;
	float metallic = anmaterial.metalness * metal_roughness_idx.b;
	float roughness = anmaterial.roughness * metal_roughness_idx.g;
	vec3 emissive = texture2D(emissive_map, f_texcoord).rgb;
	vec3 total_light = (anmaterial.emissive.rgb * emissive.rgb);

	for (uint i = uint(0); i < ACTIVE_POINTS_RAW; i++) {
		float atten = point_attenuation(i, f_position.xyz);
		float shadow = point_shadow(i, vec3(f_position));

		vec3 lum =
			mix(vec3(0.0),
				mrp_lighting(POINT_LIGHT(i).position.xyz,
				             POINT_LIGHT(i).diffuse,
				             f_position.xyz, view_dir,
				             albedo, f_normal, metallic, roughness),
				shadow*atten);

		total_light += lum;
	}

	for (uint i = uint(0); i < ACTIVE_SPOTS_RAW; i++) {
		float atten = spot_attenuation(i, f_position.xyz);
		vec3 lum = mrp_lighting(SPOT_LIGHT(i).position.xyz,
		                        SPOT_LIGHT(i).diffuse, 
		                        f_position.xyz, view_dir,
		                        albedo, f_normal, metallic, roughness);

		total_light += lum*atten;
	}

	for (uint i = uint(0); i < ACTIVE_DIRECTIONAL_RAW; i++) {
		float atten = directional_attenuation(i, f_position.xyz);
		vec3 lum = mrp_lighting(DIRECTIONAL_LIGHT(i).position.xyz,
		                        DIRECTIONAL_LIGHT(i).diffuse, 
		                        f_position.xyz, view_dir,
		                        albedo, f_normal, metallic, roughness);

		total_light += lum*atten;
	}

	FRAG_COLOR = vec4(total_light, 1.0);
}
