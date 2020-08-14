#pragma once

#include <lib/atlas.glsl>
#include <lib/compat.glsl>

vec3 textureCubeAtlasUV(vec3 dir) {
	vec3 ab = abs(dir);
	vec2 f_uv;
	int face;

	if (ab.x >= ab.y && ab.x >= ab.z) {
		face = (dir.x < 0.0)? 0 : 3;
		f_uv = vec2((dir.x > 0.0)? dir.z : -dir.z, dir.y) / ab.x;

	} else if (ab.y >= ab.z) {
		face = (dir.y < 0.0)? 1 : 4;
		f_uv = vec2((dir.y > 0.0)? dir.x : -dir.x, dir.z) / ab.y;

	} else {
		face = (dir.z < 0.0)? 2 : 5;
		f_uv = vec2((dir.z < 0.0)? dir.x : -dir.x, dir.y) / ab.z;
	}

	return vec3((f_uv + 1.0) * 0.5, float(face));
}

vec4 textureCubeAtlas(in sampler2D atlas, vec3 cube[6], vec3 dir) {
	vec3 dat = textureCubeAtlasUV(dir);
	vec2 uv = dat.xy;
	int  face = int(dat.z);

	return texture2DAtlas(atlas, cube[face], uv);
}
