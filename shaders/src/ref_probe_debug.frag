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
uniform vec3 cubeface[6];

vec4 textureAtlasCubemap(vec3 cube[6], vec3 dir) {
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

	vec2 p_uv = vec2(
		cubeface[face].z * f_uv.x + cubeface[face].x,
		cubeface[face].z * f_uv.y + cubeface[face].y
	);

	return texture2D(reflection_atlas, p_uv);
}

void main(void) {
#if 1
	FRAG_COLOR = vec4(textureAtlasCubemap(cubeface, f_normal).rgb, 1.0);
#else
	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	vec3 env = reflect(-view_dir, f_normal);
	FRAG_COLOR = vec4(textureAtlasCubemap(cubeface, env).rgb, 1.0);
#endif
}
