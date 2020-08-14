#pragma once

struct cubeUV {
	vec2 uv;
	int  face;
};

vec2 atlasUV(vec3 transform, vec2 uv) {
	return vec2(
		transform.z * uv.x + transform.x,
		transform.z * uv.y + transform.y
	);
}

vec4 texture2DAtlas(in sampler2D atlas, vec3 slice, vec2 uv) {
	// TODO: does this hurt performance?
	float inv_sz = 0.5 / (float(textureSize(atlas, 0).x) * slice.z);
	return texture2D(atlas, atlasUV(slice, clamp(uv, inv_sz, 1.0 - inv_sz)));
}
