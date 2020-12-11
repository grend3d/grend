#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define PHONG_USE_BLINN_PHONG

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>
#include <lib/atlas_cubemap.glsl>
#include <lib/shadows.glsl>

#define LIGHT_FUNCTION blinn_phong_lighting
#include <lighting/blinn-phong.glsl>
#include <lighting/fresnel-schlick.glsl>
#include <lighting/lightingLoop.glsl>

void main(void) {
	uint cluster = CURRENT_CLUSTER();

	vec4 texcolor = texture2D(diffuse_map, f_texcoord);
	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	vec3 metalrough = texture2D(specular_map, f_texcoord).rgb;
	vec3 emissive = texture2D(emissive_map, f_texcoord).rgb;

	vec3 albedo = anmaterial.diffuse.rgb * texcolor.rgb * anmaterial.diffuse.w;
	float metallic = anmaterial.metalness * metalrough.b;
	float roughness = anmaterial.roughness * metalrough.g;

	// f_texcoord also flips normal maps, so multiply the y component
	// by -1 here to flip it
	vec3 normal_dir = vec3(1, -1, 1) * normalize(normidx * 2.0 - 1.0);
	normal_dir = normalize(TBN * normal_dir);

	vec3 view_pos = vec3(v_inv * vec4(0, 0, 0, 1));
	vec3 view_dir = normalize(view_pos - f_position.xyz);
	mat4 mvp = p*v*m;

	vec4 radmap = textureCubeAtlas(irradiance_atlas, irradiance_probe, normal_dir);

	// ambient light, from irradiance map
	vec3 total_light =
		(radmap.rgb * anmaterial.diffuse.rgb * albedo * (1.0 - metallic))
		+ (anmaterial.emissive.rgb * emissive.rgb);

	LIGHT_LOOP(cluster, f_position.xyz, view_dir, albedo,
	           normal_dir, metallic, roughness, 1.0);

	//vec4 dispnorm = vec4((normal_dir + 1.0)/2.0, 1.0);
	FRAG_COLOR = vec4(total_light, anmaterial.opacity);
}
