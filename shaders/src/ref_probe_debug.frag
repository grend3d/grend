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

vec4 textureAtlasParabaloid(vec3 p[2], vec3 dir) {
	//vec3 p_uv = parabaloid[p_idx] * vec3(reflect(-view_dir, f_normal).xz, 1.0);
	//vec3 p_uv = parabaloid[p_idx] * clamp(vec3(f_normal.xz, 1.0), 0.0, 1.0);
	//vec3 adj_norm = pow((dir + 1.0)/2.0, vec3(2.0));
	vec3 adj_norm = clamp((dir + 1.0)/2.0, 0.0, 1.0);
	int p_idx = int(round(adj_norm.z));
	vec2 adjx = vec2(adj_norm.x, 1.0 - adj_norm.x);

	vec2 p_uv = vec2(
		p[p_idx].z * adjx[p_idx] + p[p_idx].x,
		p[p_idx].z * adj_norm.y + p[p_idx].y
	);

	return texture2D(reflection_atlas, p_uv.xy);
}

void main(void) {
#if 1
	FRAG_COLOR = vec4(textureAtlasParabaloid(parabaloid, f_normal).rgb, 1.0);
#else
	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	vec3 env = reflect(-view_dir, f_normal);
	FRAG_COLOR = vec4(textureAtlasParabaloid(parabaloid, env).rgb, 1.0);
#endif
}
