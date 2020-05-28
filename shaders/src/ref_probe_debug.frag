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
#include <lib/shading-varying.glsl>
#include <lib/atlas_cubemap.glsl>

uniform sampler2D reflection_atlas;
uniform vec3 cubeface[6];

void main(void) {
#if 1
	FRAG_COLOR = vec4(textureCubeAtlas(reflection_atlas, cubeface, f_normal).rgb, 1.0);
#else
	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	vec3 env = reflect(-view_dir, f_normal);
	FRAG_COLOR = vec4(textureCubeAtlas(reflection_atlas, cubeface, env).rgb, 1.0);
#endif
}
