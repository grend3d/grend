#pragma once

#include <lib/atlas.glsl>
#include <lib/compat.glsl>

vec4 textureCubeAtlas(in sampler2D atlas, vec3 cube[6], vec3 dir) {
	vec3 adj = clamp((normalize(dir) + 1.0)/2.0, 0.0, 1.0);
	vec3 ab = abs(dir);
	vec2 f_uv;
	int face;

	if (ab.z >= ab.x && ab.z >= ab.y) {
		face = (adj.z > 0.5)? 5 : 2;
		f_uv = vec2((adj.z < 0.5)? adj.x : (1.0 - adj.x), adj.y);

	} else if (ab.y >= ab.x) {
		face = (adj.y > 0.5)? 4 : 1;
		f_uv = vec2((adj.y > 0.5)? adj.x : (1.0 - adj.x), adj.z);

	} else {
		face = (adj.x > 0.5)? 3 : 0;
		f_uv = vec2((adj.x > 0.5)? adj.z : (1.0 - adj.z), adj.y);
	}

	return texture2D(atlas, atlasUV(cube[face], f_uv));
}
