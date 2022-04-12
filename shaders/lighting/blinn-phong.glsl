#pragma once
#include <lib/shading-uniforms.glsl>
#include <lib/utility.glsl>
#include <lib/constants.glsl>
#include <lighting/lambert-diffuse.glsl>
#include <lighting/fresnel-schlick.glsl>

vec3 blinn_phong_lighting(vec3 light_pos, vec4 light_color, vec3 pos, vec3 view,
                          vec3 albedo, vec3 normal, float metallic, float roughness)
{
	vec3 light_dir = normalize(light_pos - pos);

	// TODO: albedo/shininess could be precalculated, no reason to do this
	//       for every light
	vec3 f_0 = mix(albedo, vec3(0.04), metallic);
	float adjShiny = clamp((1.0 - roughness*roughness), 0.1, 1.0);

#ifdef PHONG_USE_BLINN_PHONG
	vec3 half_dir = normalize(light_dir + view);
	vec3 Fa = F(f_0, view, half_dir);
	vec3 adjAlbedo = (1.0 - Fa) * (f_0 / PI);

	vec3 diffuse = light_color.rgb * adjAlbedo.rgb;

	float angle = mindot(half_dir, normal);
	vec3 specular =
		Fa * albedo.rgb * light_color.rgb * light_color.w
		* pow(angle, adjShiny*4.0);

#else
	vec3 adjAlbedo = (f_0 / PI);
	vec3 diffuse = light_color.rgb * adjAlbedo.rgb;

	float angle = mindot(reflect(-light_dir, normal), view);
	vec3 specular =
		albedo.rgb * light_color.rgb * light_color.w
		* pow(angle, adjShiny*4.0);
#endif

	float factor = lambert_diffuse(0.5, light_dir, normal, view);
	return factor * (diffuse + specular);
}

