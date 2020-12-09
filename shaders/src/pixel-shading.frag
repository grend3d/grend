#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define PHONG_USE_BLINN_PHONG

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>
#include <lib/atlas_cubemap.glsl>
#include <lib/shadows.glsl>
#include <lighting/blinn-phong.glsl>
#include <lighting/fresnel-schlick.glsl>

void main(void) {
	uint cluster = CURRENT_CLUSTER();

	vec4 texcolor = texture2D(diffuse_map, f_texcoord);
	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	vec3 metalrough = texture2D(specular_map, f_texcoord).rgb;
	vec3 emissive = texture2D(emissive_map, f_texcoord).rgb;

	vec3 albedo = anmaterial.diffuse.rgb * texcolor.rgb * anmaterial.diffuse.w;
	float metallic = anmaterial.metalness * metalrough.b;
	float roughness = anmaterial.roughness * metalrough.g;
	float shininess = (1.0 - roughness*roughness);

	// f_texcoord also flips normal maps, so multiply the y component
	// by -1 here to flip it
	vec3 normal_dir = vec3(1, -1, 1) * normalize(normidx * 2.0 - 1.0);
	normal_dir = normalize(TBN * normal_dir);

	vec3 view_pos = vec3(v_inv * vec4(0, 0, 0, 1));
	vec3 view_dir = normalize(view_pos - f_position.xyz);
	mat4 mvp = p*v*m;

	vec4 radmap = textureCubeAtlas(irradiance_atlas, irradiance_probe, normal_dir);

	// ambient light, from irradiance map
	vec3 total_light =
		(radmap.rgb * anmaterial.diffuse.rgb * albedo * (1.0 - metallic))
		+ (anmaterial.emissive.rgb * emissive.rgb);

	for (uint i = 0u; i < ACTIVE_POINTS(cluster); i++) {
		float atten = point_attenuation(i, cluster, f_position.xyz);
		float shadow = point_shadow(i, cluster, vec3(f_position));

		vec3 lum =
			blinn_phong_lighting(POINT_LIGHT(i, cluster).position,
			                     POINT_LIGHT(i, cluster).diffuse, f_position.xyz,
			                     view_dir, albedo, normal_dir, metallic, shininess);

		total_light += lum*atten*shadow;
	}

	for (uint i = 0u; i < ACTIVE_SPOTS(cluster); i++) {
		float atten = spot_attenuation(i, cluster, f_position.xyz);
		float shadow = spot_shadow(i, cluster, vec3(f_position));

		vec3 lum =
			blinn_phong_lighting(SPOT_LIGHT(i, cluster).position,
			                     SPOT_LIGHT(i, cluster).diffuse, f_position.xyz,
			                     view_dir, albedo, normal_dir, metallic, shininess);

		total_light += lum*atten*shadow;
	}

	for (uint i = 0u; i < ACTIVE_DIRECTIONAL(cluster); i++) {
		float atten = directional_attenuation(i, cluster, vec3(f_position));
		vec3 lum =
			blinn_phong_lighting(DIRECTIONAL_LIGHT(i, cluster).position,
			                     DIRECTIONAL_LIGHT(i, cluster).diffuse, f_position.xyz,
			                     view_dir, albedo, normal_dir, metallic, shininess);

		total_light += lum*atten;
	}

	//vec4 dispnorm = vec4((normal_dir + 1.0)/2.0, 1.0);
	FRAG_COLOR = vec4(total_light, anmaterial.opacity);
}
