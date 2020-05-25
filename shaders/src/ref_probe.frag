#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define PI 3.1415926

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1
#define ENABLE_REFRACTION 1

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>

uniform sampler2D reflection_atlas;
uniform vec3 parabaloid[2];

void main(void) {
	vec3 adj_norm = clamp((dir + 1.0)/2.0, 0.0, 1.0);
	FRAG_COLOR = vec4(adj_norm, 1.0);
}
