#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define PI 3.1415926

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1
#define ENABLE_REFRACTION 1

#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>
#include <lighting/metal-roughness-pbr.glsl>

void main(void) {
	float time_s = time_ms / 1000.0;
	vec2 t_coord = f_texcoord + time_s/2.0;

	vec3 normidx = texture2D(normal_map, t_coord).rgb;
	vec3 ambient_light = vec3(0.1);
	// TODO: normal mapping still borked
	vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));

	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	mat4 mvp = p*v*m;

	vec3 metal_roughness_idx = texture2D(specular_map, t_coord).rgb;

	vec3 albedo = texture2D(diffuse_map, t_coord).rgb;
	float metallic = anmaterial.metalness * metal_roughness_idx.b;
	float roughness = anmaterial.roughness * metal_roughness_idx.g;
	float aoidx = pow(texture2D(ambient_occ_map, t_coord).r, 2.0);
	//float aoidx = texture2D(ambient_occ_map, t_coord).r;

	vec3 k = vec3(sin(time_s), cos(time_s), 0.5);
	vec3 greyish = vec3(0.7, 0.7, 0.7);
	//vec3 total_light = mix(greyish, k * albedo, roughness);
	vec3 total_light = k * albedo;

	vec4 displight = vec4(total_light, 0.33 + (sin(time_s*0.77) + 1)/3.0);
	gl_FragColor = displight;
	//gl_FragColor = dispnorm;
}
