#pragma once

vec2 atlasUV(vec3 transform, vec2 uv) {
	return vec2(
		transform.z * uv.x + transform.x,
		transform.z * uv.y + transform.y
	);
}

vec4 texture2DAtlas(in sampler2D atlas, vec3 slice, vec2 uv) {
	return texture2D(atlas, atlasUV(slice, uv));
}
