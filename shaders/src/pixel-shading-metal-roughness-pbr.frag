#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1
#define ENABLE_REFRACTION 1

// TODO: maybe make this a uniform (probably not though)
#define NORMAL_WEIGHT 0.85
//#define MRP_USE_SCHLICK_GGX
//#define MRP_USE_LAMBERT_DIFFUSE

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>
#include <lib/shadows.glsl>
#include <lib/tonemapping.glsl>
#include <lib/atlas_cubemap.glsl>
#include <lib/constants.glsl>
// TODO: actually, rename this to parallax-correct, or probe-parallax-correct?
#include <lib/parallax-cube.glsl>
#include <lighting/metal-roughness-pbr.glsl>

void main(void) {
	uint cluster = CURRENT_CLUSTER();

	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	vec3 ambient_light = vec3(0.1);
	// TODO: normal mapping still borked
	//vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));
	vec3 normal_dir = normalize(normidx * 2.0 - 1.0);
	//normal_dir = normalize(TBN * normal_dir);
	normal_dir = mix(f_normal, normalize(TBN * normal_dir), NORMAL_WEIGHT);

	vec3 view_pos = vec3(v_inv * vec4(0, 0, 0, 1));
	vec3 view_dir = normalize(view_pos - f_position.xyz);
	mat4 mvp = p*v*m;

	vec3 metal_roughness_idx = texture2D(specular_map, f_texcoord).rgb;
	vec4 texcolor = texture2D(diffuse_map, f_texcoord);
	vec4 radmap = textureCubeAtlas(irradiance_atlas, irradiance_probe, normal_dir);
	vec3 albedo = texcolor.rgb;
	float opacity = texcolor.a;

	float metallic = anmaterial.metalness * metal_roughness_idx.b;
	float roughness = anmaterial.roughness * metal_roughness_idx.g;
	float aoidx = pow(texture2D(ambient_occ_map, f_texcoord).r, 2.0);
	//float aoidx = texture2D(ambient_occ_map, f_texcoord).r;

	//vec3 total_light = vec3(0);
	vec3 total_light = (roughness*roughness)*radmap.rgb;

	for (uint i = 0u; i < ACTIVE_POINTS; i++) {
		float atten = point_attenuation(i, cluster, vec3(f_position));
		float shadow = point_shadow(i, cluster, vec3(f_position));

		vec3 lum =
			mix(vec3(0.0),
			    mrp_lighting(POINT_LIGHT(i, cluster).position,
				             POINT_LIGHT(i, cluster).diffuse, 
		                     vec3(f_position), view_dir,
		                     albedo, normal_dir, metallic, roughness),
				shadow);

		total_light += lum*atten*aoidx;
	}

	for (uint i = 0u; i < ACTIVE_SPOTS; i++) {
		float atten = spot_attenuation(i, cluster, vec3(f_position));
		float shadow = spot_shadow(i, cluster, vec3(f_position));
		vec3 lum =
			mix(vec3(0.0),
			    mrp_lighting(SPOT_LIGHT(i, cluster).position,
				             SPOT_LIGHT(i, cluster).diffuse,
			                 vec3(f_position), view_dir,
			                 albedo, normal_dir, metallic, roughness),
			shadow
		);

		total_light += lum*atten*aoidx;
	}

	for (uint i = 0u; i < ACTIVE_DIRECTIONAL; i++) {
		float atten = directional_attenuation(i, cluster, vec3(f_position));
		vec3 lum = mrp_lighting(
			vec3(f_position) - DIRECTIONAL_LIGHT(i, cluster).direction,
			DIRECTIONAL_LIGHT(i, cluster).diffuse,
			vec3(f_position), view_dir,
			albedo, normal_dir, metallic, roughness);

		total_light += lum*atten*aoidx;
	}

	float a = alpha(roughness);
/*
	//float a = roughness;
	// TODO: s/8.0/max LOD/
	vec3 refdir = reflect(-view_dir, normal_dir);
	//vec3 env = vec3(textureLod(skytexture, refdir, 8.0*a));
	// TODO: mipmap
	vec3 env = textureCubeAtlas(reflection_atlas, reflection_probe, refdir).rgb;
	vec3 Fb = F(f_0(albedo, metallic), view_dir, normalize(view_dir + refdir));
*/

	//mat4 minv = inverse(m)*v_inv;
	//mat4 minv = inverse(v);
	//vec3 posws = vec3((minv * f_position));
	//vec3 posws = vec3(v_inv*f_position);
	vec3 posws = f_position.xyz;

	//vec3 refdir = reflect(-view_dir, normal_dir);
	vec3 refdir = correctParallax(posws, view_pos, normal_dir);
	vec3 env = textureCubeAtlas(reflection_atlas, reflection_probe, refdir).rgb;
	vec3 Fb = F(f_0(albedo, metallic), view_dir, normalize(view_dir + refdir));

	//total_light += env * Fb;
	total_light += 0.8 * (1.0 - a) * env * Fb;

#if ENABLE_REFRACTION
	vec3 ref_light = vec3(0);

	if (anmaterial.opacity < 1.0) {
		// TODO: handle refraction indexes
		vec3 dir = refract(-view_dir, normal_dir, 1.0/1.5);
		vec3 ref = textureCubeAtlas(reflection_atlas, reflection_probe, dir).rgb;
		//ref_light = anmaterial.diffuse.xyz * 0.5*ref;
		ref_light = 0.9*ref;
	}
/*
	if (anmaterial.opacity < 1.0) {
		ref_light = anmaterial.diffuse.xyz
			* 0.5*vec3(textureCube(skytexture,
			                       refract(-view_dir, normal_dir, 1.0/1.5)));
	}
*/

	total_light += ref_light;
#endif

	// apply tonemapping here if there's no postprocessing step afterwards
	total_light = EARLY_TONEMAP(total_light, 1.0);
	FRAG_COLOR = vec4(total_light, opacity);

	//vec4 dispnorm = vec4((normal_dir + 1.0)/2.0, 1.0);
	//gl_FragColor = dispnorm;
}
