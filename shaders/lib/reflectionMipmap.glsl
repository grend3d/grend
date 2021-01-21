#pragma once
#include <lib/atlas_cubemap.glsl>

vec4 reflectionLinearMip(vec3 pos, vec3 cam, vec3 normal, float roughness) {
	float radj = clamp(roughness, 0.0, 1.0);
	// TODO: configurable mip levels
	float amount = 4.0 * radj;
	int lower = int(floor(amount));
	int upper = int(ceil(amount));
	vec3 dir = correctParallax(pos, cam, normal);

	vec3 lowercube[6] = vec3[] (
		reflection_probe[6*lower + 0].xyz,
		reflection_probe[6*lower + 1].xyz,
		reflection_probe[6*lower + 2].xyz,
		reflection_probe[6*lower + 3].xyz,
		reflection_probe[6*lower + 4].xyz,
		reflection_probe[6*lower + 5].xyz
	);

	vec4 lowerSample;
	if (lower == 0) {
		lowerSample =
			textureCubeAtlas(reflection_atlas, lowercube, dir);

	} else {
		lowerSample =
			textureCubeAtlas(irradiance_atlas, lowercube, dir);
	}
	
	if (lower == upper) {
		// return exact sample, no need to interpolate
		return lowerSample;

	} else {
		vec3 uppercube[6] = vec3[] (
			reflection_probe[6*upper + 0].xyz,
			reflection_probe[6*upper + 1].xyz,
			reflection_probe[6*upper + 2].xyz,
			reflection_probe[6*upper + 3].xyz,
			reflection_probe[6*upper + 4].xyz,
			reflection_probe[6*upper + 5].xyz
		);

		vec4 upperSample = textureCubeAtlas(irradiance_atlas, uppercube, dir);
		return mix(lowerSample, upperSample, fract(amount));
	}
}
