#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>

void main(void) {
	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));

	vec2 matuva = vec2(v * vec4(normal_dir, 0))*0.5 + vec2(0.5);
	vec2 matuv  = vec2(matuva.x, 1.0 - matuva.y);

	vec3 matcolor = texture2D(diffuse_map, matuv).rgb;
	vec3 total_light = matcolor;

	FRAG_COLOR = vec4(total_light, 1.0);
}
