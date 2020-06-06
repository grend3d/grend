#pragma once
#include <lib/shading-uniforms.glsl>
#include <lib/atlas_cubemap.glsl>

// TODO: make these uniforms
float near = 0.1;
float far = 100.0;

// https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap
float vec_to_depth(vec3 vec) {
	vec3 ab = abs(vec);
	float z_comp = max(ab.x, max(ab.y, ab.z));
	float norm_z_comp = (far+near) / (far-near) - (2.0*far*near)/(far-near)/z_comp;

	return (norm_z_comp + 1.0) * 0.5;
}

float point_shadow(int i, vec3 pos) {
	// TODO: can branching be avoided without the texture sample?
	if (!point_lights[i].casts_shadows) {
		return 1.0;
	}

	vec3 light_vertex = point_lights[i].position - pos;
	vec3 light_dir = normalize(light_vertex);
	vec4 depth = textureCubeAtlas(shadowmap_atlas,
								  point_lights[i].shadowmap, -light_dir);

	return ((depth.r + 0.00001) > vec_to_depth(light_vertex))? 1.0 : 0.0;
}

float spot_shadow(int i, vec3 pos) {
	if (!spot_lights[i].casts_shadows) {
		return 1.0;
	}

	vec3 light_vertex = spot_lights[i].position - pos;
	vec3 light_dir = normalize(light_vertex);

	if (dot(light_dir, spot_lights[i].direction) > spot_lights[i].angle) {
		// XXX: maybe pass this in a uniform, or use quarternion for rotation
		//      and extract an SO(3) out of that
		vec3 adjdir = normalize(-spot_lights[i].direction);
		vec3 right = normalize(cross(vec3(0, 0, 1), adjdir));
		vec3 up = normalize(cross(right, adjdir));

		float p = 1.0 - spot_lights[i].angle;
		//vec2 uv = (vec2(light_dir.x, -light_dir.z) + 1.0) / 2.0;
		vec2 uv = (vec2(dot(light_dir, right), dot(light_dir, up)) + 1.0) / 2.0;
		//vec2 uv = (vec2(dot(light_dir, right), dot(light_dir, up)) + 1.0) / 2.0;
		vec4 depth = texture2DAtlas(shadowmap_atlas,
		                            spot_lights[i].shadowmap, uv);

		return ((depth.r + 0.001) > vec_to_depth(light_vertex))? 1.0 : 0.0;
	}

	return 0.0;
}

float directional_shadow(int i, vec3 pos) {
	if (!directional_lights[i].casts_shadows) {
		return 1.0;
	}

	// TODO: directional_shadow()
	return 1.0;
}
