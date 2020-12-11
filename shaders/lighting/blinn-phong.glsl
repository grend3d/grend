#pragma once
#include <lib/shading-uniforms.glsl>
#include <lib/utility.glsl>
#include <lib/constants.glsl>
#include <lighting/lambert-diffuse.glsl>

vec3 blinn_phong_lighting(vec3 light_pos, vec4 light_color, vec3 pos, vec3 view,
                          vec3 albedo, vec3 normal, float metallic, float roughness)
{
	// TODO: albedo/shininess could be precalculated, no reason to do this
	//       for every light
	vec3 adjAlbedo = mix(albedo, vec3(0.04), metallic);
	float adjShiny = clamp((1.0 - roughness*roughness), 0.25, 1.0);
	vec3 light_dir = normalize(light_pos - pos);
	float factor = lambert_diffuse(0.5, light_dir, normal, view);

	vec3 diffuse = light_color.rgb * adjAlbedo.rgb * INV_PI * factor;

#ifdef PHONG_USE_BLINN_PHONG
	float angle = mindot(normalize(light_dir + view), normal);
	vec3 specular =
		albedo.rgb * light_color.rgb * light_color.w
		* pow(angle, adjShiny*8.0);

#else
	float angle = mindot(reflect(-light_dir, normal), view);
	vec3 specular =
		albedo.rgb * light_color.rgb * light_color.w
		* pow(angle, adjShiny*4.0);
#endif

	return diffuse + specular;
}

