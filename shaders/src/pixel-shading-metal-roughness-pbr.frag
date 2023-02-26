#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

//#define MRP_USE_SCHLICK_GGX
//#define MRP_USE_LAMBERT_DIFFUSE
//#define DEBUG_CLUSTERS

//#define NO_POSTPROCESSING 1
#define LIGHT_FUNCTION mrp_lighting

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/tonemapping.glsl>
#include <lib/atlas_cubemap.glsl>
// TODO: actually, rename this to parallax-correct, or probe-parallax-correct?
#include <lib/parallax-cube.glsl>
#include <lighting/metal-roughness-pbr.glsl>
#include <lighting/lightingLoop.glsl>
#include <lib/reflectionMipmap.glsl>

#ifdef NO_POSTPROCESSING
#include <lib/camera-uniforms.glsl>
#endif

vec3 refFactor(vec3 view_dir, vec3 normal, vec3 albedo, float metalness, float roughness)
{
	// calculate fresnel factor using the reflection direction as the light direction
	vec3 ref_dir  = reflect(-view_dir, normal);
	vec3 half_dir = normalize(view_dir + ref_dir);

	return F(f_0(albedo, metalness), view_dir, half_dir) * 0.8;
}

#ifdef DEBUG_CLUSTERS
vec3 debugColor(uint cluster) {
	float N = float(ACTIVE_POINTS(cluster)) / float(MAX_LIGHTS);
	const float thresh = 0.75;
	const float invthresh = thresh / 1.0;
	return vec3(invthresh * max(0.0, N - thresh), 1.0 - N, N);
}
#endif

void main(void) {
	uint cluster = CURRENT_CLUSTER();

	vec4  texcolor            = texture2D(diffuse_map, f_texcoord);
	vec3  emissive            = texture2D(emissive_map, f_texcoord).rgb;
	vec3  metal_roughness_idx = texture2D(specular_map, f_texcoord).rgb;
	float aoidx               = texture2D(ambient_occ_map, f_texcoord).r;
	vec3  lightmapped         = texture2D(lightmap, f_lightmap).rgb;
	vec3  normidx             = texture2D(normal_map, f_texcoord).rgb;

	float opacity = texcolor.a * anmaterial.opacity;

	// convert from sRGB color space to linear color space
	texcolor.rgb    = pow(texcolor.rgb,    vec3(2.2));
	lightmapped.rgb = pow(lightmapped.rgb, vec3(2.2));
	emissive.rgb    = pow(emissive.rgb,    vec3(2.2));

	// XXX: assume that the glsl compiler will remove constantly false branches
	//      easy optimization, so should be a safe bet...
	// TODO: do own preprocessing that eliminates these, or better yet,
	//       write my own darn shading language already
	// TODO: move these to macros
	if (BLEND_MODE_MASKED == 1) {
		if (opacity < anmaterial.alphaCutoff) {
			discard;
		}
	}

	if (BLEND_MODE_DITHERED_BLEND == 1) {
		float noise = hash12(gl_FragCoord.xy*1.0);
		if (opacity < noise) {
			discard;
		}
	}

	vec3 view_pos = vec3(v_inv * vec4(0, 0, 0, 1));
	vec3 view_vec = view_pos - f_position.xyz;
	vec3 view_dir = normalize(view_vec);
	vec3 normal   = normalize(TBN * normalize(normidx * 2.0 - 1.0));

	vec3 albedo = f_color.rgb
	            * anmaterial.diffuse.rgb
	            * texcolor.rgb
	            * anmaterial.diffuse.w
	            * f_color.w;

	float metallic  = anmaterial.metalness * metal_roughness_idx.b;
	float roughness = anmaterial.roughness * metal_roughness_idx.g;

	vec4 radmap = textureCubeAtlas(irradiance_atlas, irradiance_probe, normal);
	vec3 refmap = reflectionLinearMip(f_position.xyz, view_pos, normal, roughness).rgb;
	vec3 reffac = refFactor(view_dir, normal, albedo, metallic, roughness);

	// calculate lighting for irradiance probe
	vec3 half_dir = normalize(view_dir + normal);
	vec3 Fspec = F(f_0(albedo, metallic), view_dir, half_dir);
	vec3 Fdiff = 1.0 - Fspec;
	vec3 irrad = (radmap.rgb * anmaterial.diffuse.rgb * albedo * Fdiff * aoidx);

	// initial ambient/emissive light
	vec3 total_light =
	      irrad
	    + (anmaterial.emissive.rgb * emissive.rgb)
	    + lightmapped*Fdiff;

	LIGHT_LOOP(cluster,
	           f_position.xyz,
	           view_dir,
	           albedo,
	           normal,
	           metallic,
	           roughness,
	           aoidx);

	total_light += refmap*reffac;

	// apply tonemapping here if there's no postprocessing step afterwards
	total_light = vec3(EARLY_TONEMAP(vec4(total_light, 1.0), exposure, f_texcoord));

	#if GREND_USE_G_BUFFER
		FRAG_NORMAL      = normal;
		FRAG_POSITION    = f_position.xyz;
		FRAG_METAL_ROUGH = vec3(metallic, roughness, 0);
		FRAG_RENDER_ID   = renderID;
	#endif

	#ifdef DEBUG_CLUSTERS
		FRAG_COLOR = vec4(total_light + debugColor(cluster), opacity);
	#else
		FRAG_COLOR = vec4(total_light, opacity);
	#endif
}
