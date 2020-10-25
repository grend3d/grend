#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1
#define ENABLE_REFRACTION 0

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>
#include <lib/atlas_cubemap.glsl>
#include <lib/shadows.glsl>
#include <lighting/blinn-phong.glsl>
#include <lighting/fresnel-schlick.glsl>

void main(void) {
	uint cluster = CURRENT_CLUSTER();

	vec3 albedo = texture2D(diffuse_map, f_texcoord).rgb;
	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	vec2 metalrough = texture2D(specular_map, f_texcoord).rg;
	float metallic = metalrough.r;
	float roughness = metalrough.g;
	float specidx = (1.0 - roughness);

	vec3 ambient_light = vec3(0.0);
	//vec3 normal_dir = normalize(f_normal);
	//mat3 TBN = transpose(mat3(f_tangent, f_bitangent, f_normal));
	//vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));
	// f_texcoord also flips normal maps, so multiply the y component
	// by -1 here to flip it
	vec3 normal_dir = vec3(1, -1, 1) * normalize(normidx * 2.0 - 1.0);
	normal_dir = normalize(TBN * normal_dir);

	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	mat4 mvp = p*v*m;
	vec3 total_light = vec3(anmaterial.diffuse.x * ambient_light.x,
	                        anmaterial.diffuse.y * ambient_light.y,
	                        anmaterial.diffuse.z * ambient_light.z);

	for (uint i = 0u; i < ACTIVE_POINTS; i++) {
		float atten = point_attenuation(i, cluster, f_position.xyz);
		float shadow = point_shadow(i, cluster, vec3(f_position));

		vec3 lum =
			blinn_phong_lighting(POINT_LIGHT(i, cluster).position,
				POINT_LIGHT(i, cluster).diffuse, f_position.xyz,
				view_dir, albedo, normal_dir, specidx);

		total_light += lum*atten*shadow;
	}

	for (uint i = 0u; i < ACTIVE_SPOTS; i++) {
		float atten = spot_attenuation(i, cluster, f_position.xyz);
		float shadow = spot_shadow(i, cluster, vec3(f_position));

		vec3 lum =
			blinn_phong_lighting(SPOT_LIGHT(i, cluster).position,
				SPOT_LIGHT(i, cluster).diffuse, f_position.xyz,
				view_dir, albedo, normal_dir, specidx);

		total_light += lum*atten*shadow;
	}

	for (uint i = 0u; i < ACTIVE_DIRECTIONAL; i++) {
		float atten = directional_attenuation(i, cluster, vec3(f_position));
		vec3 lum =
			blinn_phong_lighting(DIRECTIONAL_LIGHT(i, cluster).position,
				DIRECTIONAL_LIGHT(i, cluster).diffuse, f_position.xyz,
				view_dir, albedo, normal_dir, specidx);

		total_light += lum*atten;
	}


#if ENABLE_REFRACTION
	vec3 ref_light = vec3(0);

	if (anmaterial.opacity < 1.0) {
		ref_light = anmaterial.diffuse.xyz
			* 0.5*vec3(textureCube(skytexture,
						refract(-view_dir, normal_dir, 1.0/1.5)));
	}
#endif

	vec3 refdir = reflect(-view_dir, normal_dir);
	vec3 env = textureCubeAtlas(reflection_atlas, reflection_probe, refdir).rgb;
	vec3 Fb = F(albedo, view_dir, normalize(view_dir + refdir));
	total_light += 0.5 * specidx * env * Fb;

	//vec4 dispnorm = vec4((normal_dir + 1.0)/2.0, 1.0);
	FRAG_COLOR = vec4(total_light, anmaterial.opacity);
}
