#pragma once

// TODO: returns [-0.5, 0.5], maybe should change this to [-1, 1]
vec2 uniformNoise(in vec2 uv, float scale) {
	return
	fract((4096.0 + scale) * vec2(
			cos(uv.x * 55117.0 + uv.y * 1000211.0 + 1.0),
			sin(uv.y * 50053.0 + uv.x * 1000187.0 + 1.1)
		)) - 0.5;
}
