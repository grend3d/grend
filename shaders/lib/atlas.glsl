#pragma once

vec2 atlasUV(vec3 transform, vec2 uv) {
	return vec2(
		transform.z * uv.x + transform.x,
		transform.z * uv.y + transform.y
	);
}
