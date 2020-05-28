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
	vec3 light_vertex = point_lights[i].position - vec3(f_position);
	vec3 light_dir = normalize(light_vertex);
	vec4 depth = textureCubeAtlas(shadowmap_atlas,
								  point_lights[i].shadowmap, -light_dir);

	return ((depth.r + 0.00001) > vec_to_depth(light_vertex))? 1.0 : 0.0;
}

float spot_shadow(int i, vec3 pos) {
	// TODO: spot_shadow
	return 0.0;
}

float directional_shadow(int i, vec3 pos) {
	// TODO: directional_shadow()
	return 0.0;
}
