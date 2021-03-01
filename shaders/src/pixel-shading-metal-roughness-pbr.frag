#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

// TODO: maybe make this a uniform (probably not though)
#define NORMAL_WEIGHT 0.85
//#define MRP_USE_SCHLICK_GGX
//#define MRP_USE_LAMBERT_DIFFUSE
//#define DEBUG_CLUSTERS

#define LIGHT_FUNCTION mrp_lighting

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
#include <lighting/lightingLoop.glsl>
#include <lib/reflectionMipmap.glsl>

void main(void) {
	uint cluster = CURRENT_CLUSTER();

	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));

	vec3 view_pos = vec3(v_inv * vec4(0, 0, 0, 1));
	vec3 view_dir = normalize(view_pos - f_position.xyz);
	mat4 mvp = p*v*m;

	vec3 metal_roughness_idx = texture2D(specular_map, f_texcoord).rgb;
	vec4 texcolor = texture2D(diffuse_map, f_texcoord);
	vec4 radmap = textureCubeAtlas(irradiance_atlas, irradiance_probe, normal_dir);

	vec3 albedo = f_color.rgb
		* anmaterial.diffuse.rgb
		* texcolor.rgb
		* anmaterial.diffuse.w * f_color.w;

	float opacity = texcolor.a * anmaterial.opacity;

	float metallic = anmaterial.metalness * metal_roughness_idx.b;
	float roughness = anmaterial.roughness * metal_roughness_idx.g;
	float aoidx = pow(texture2D(ambient_occ_map, f_texcoord).r, 2.0);
	vec3 emissive = texture2D(emissive_map, f_texcoord).rgb;
	vec3 lightmapped = texture2D(lightmap, f_lightmap).rgb;

	vec3 Fspec = F(f_0(albedo, metallic), view_dir, normalize(view_dir + normal_dir));
	vec3 Fdiff = 1.0 - Fspec;

	// ambient light, from irradiance map
	vec3 total_light =
		(radmap.rgb * anmaterial.diffuse.rgb * albedo * Fdiff * aoidx)
		+ (anmaterial.emissive.rgb * emissive.rgb)
		+ lightmapped;

	LIGHT_LOOP(cluster, f_position.xyz, view_dir, albedo,
	           normal_dir, metallic, roughness, aoidx);

	float a = alpha(roughness);
	vec3 posws = f_position.xyz;

	vec3 altdir = reflect(-view_dir, normal_dir);
	vec3 refdir = correctParallax(posws, view_pos, normal_dir);
	//vec3 env = textureCubeAtlas(reflection_atlas, test, refdir).rgb;
	vec3 env = reflectionLinearMip(posws, view_pos, normal_dir, roughness).rgb;
	vec3 Fb = F(f_0(albedo, metallic), view_dir, normalize(view_dir + altdir));
	total_light += 0.5 * (1.0 - max(0.0, a - 0.5)) * env * Fb;

	// TODO: leave refraction in? might be better as a seperate shader
	/*
	vec3 test[6] = vec3[](
		reflection_probe[0].xyz,
		reflection_probe[1].xyz,
		reflection_probe[2].xyz,
		reflection_probe[3].xyz,
		reflection_probe[4].xyz,
		reflection_probe[5].xyz
	);

	vec3 ref_light = vec3(0);

	if (anmaterial.opacity < 1.0) {
		// TODO: handle refraction indexes
		vec3 dir = refract(-view_dir, normal_dir, 1.0/1.5);
		vec3 ref = textureCubeAtlas(reflection_atlas, test, dir).rgb;
		ref_light = 0.9*ref;
	}

	total_light += ref_light;
*/

	// apply tonemapping here if there's no postprocessing step afterwards
	total_light = EARLY_TONEMAP(total_light, 1.0);

#ifdef DEBUG_CLUSTERS
	float N = float(ACTIVE_POINTS(cluster)) / float(MAX_LIGHTS);
	const float thresh = 0.75;
	const float invthresh = thresh / 1.0;
	FRAG_COLOR = vec4(total_light +
		vec3(invthresh * max(0.0, N - thresh), 1.0 - N, N), opacity);
#else
	FRAG_COLOR = vec4(total_light, opacity);
#endif
}
