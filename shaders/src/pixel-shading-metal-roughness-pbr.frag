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
#include <lib/attenuation.glsl>
#include <lib/shadows.glsl>
#include <lib/tonemapping.glsl>
#include <lib/atlas_cubemap.glsl>
#include <lighting/metal-roughness-pbr.glsl>

void main(void) {
	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	vec3 ambient_light = vec3(0.1);
	// TODO: normal mapping still borked
	//vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));
	vec3 normal_dir = vec3(1, -1, 1) * normalize(normidx * 2.0 - 1.0);
	normal_dir = normalize(TBN * normal_dir);

	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	mat4 mvp = p*v*m;

	vec3 metal_roughness_idx = texture2D(specular_map, f_texcoord).rgb;
	vec4 texcolor = texture2D(diffuse_map, f_texcoord);
	vec3 albedo = texcolor.rgb;
	float opacity = texcolor.a;

	float metallic = anmaterial.metalness * metal_roughness_idx.b;
	float roughness = anmaterial.roughness * metal_roughness_idx.g;
	float aoidx = pow(texture2D(ambient_occ_map, f_texcoord).r, 2.0);
	//float aoidx = texture2D(ambient_occ_map, f_texcoord).r;

	vec3 total_light = vec3(0);

	for (int i = 0; i < active_point_lights; i++) {
		float atten = point_attenuation(i, vec3(f_position));
		float shadow = point_shadow(i, vec3(f_position));

		vec3 lum =
			mix(vec3(0.0),
			    mrp_lighting(point_lights[i].position, point_lights[i].diffuse, 
		                     vec3(f_position), view_dir,
		                     albedo, normal_dir, metallic, roughness),
				shadow);

		total_light += lum*atten*aoidx;
	}

	for (int i = 0; i < active_spot_lights; i++) {
		float atten = spot_attenuation(i, vec3(f_position));
		float shadow = spot_shadow(i, vec3(f_position));
		vec3 lum =
			mix(vec3(0.0),
			    mrp_lighting(spot_lights[i].position, spot_lights[i].diffuse,
			                 vec3(f_position), view_dir,
			                 albedo, normal_dir, metallic, roughness),
			shadow
		);

		total_light += lum*atten*aoidx;
	}

	for (int i = 0; i < active_directional_lights; i++) {
		float atten = directional_attenuation(i, vec3(f_position));
		vec3 lum = mrp_lighting(directional_lights[i].position,
		                        directional_lights[i].diffuse,
		                        vec3(f_position), view_dir,
		                        albedo, normal_dir, metallic, roughness);

		total_light += lum*atten*aoidx;
	}

	float a = alpha(roughness);
	//float a = roughness;
	// TODO: s/8.0/max LOD/
	vec3 refdir = reflect(-view_dir, normal_dir);
	//vec3 env = vec3(textureLod(skytexture, refdir, 8.0*a));
	// TODO: mipmap
	vec3 env = textureCubeAtlas(reflection_atlas, reflection_probe, refdir).rgb;
	vec3 Fb = F(f_0(albedo, metallic), view_dir, normalize(view_dir + refdir));

	total_light += 0.5 * (1.0 - a) * env * Fb;

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
