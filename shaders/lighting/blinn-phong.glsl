#pragma once
#include <lib/shading-uniforms.glsl>
#include <lib/utility.glsl>
#include <lib/constants.glsl>
#include <lighting/lambert-diffuse.glsl>

vec3 blinn_phong_lighting(vec3 light_pos, vec4 light_color, vec3 pos, vec3 view,
                          vec3 albedo, vec3 normal, float metallic)
{
	vec3 light_dir;
	float dist = distance(light_pos, pos);
	light_dir = normalize(light_pos - pos);

	vec3 diffuse_reflection = vec3(0.0);
	vec3 specular_reflection = vec3(0);
	// TODO: once we have light probes and stuff
	vec3 env_light = vec3(0);
	float metal = metallic*anmaterial.metalness;

	diffuse_reflection =
		vec3(light_color) * vec3(albedo)
			* INV_PI*lambert_diffuse(0.5, light_dir, normal, view);

	if (metal > 0.1 && dot(normal, light_dir) >= 0.0) {
		specular_reflection = anmaterial.specular.w
			* vec3(anmaterial.specular)
			// TODO: modular specular too
			* pow(max(0.0, dot(reflect(-light_dir, normal), view)), metal);
	}

	return diffuse_reflection + specular_reflection;
}

