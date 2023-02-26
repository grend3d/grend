#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define PHONG_USE_BLINN_PHONG
#define DEBUG_CLUSTERS 0

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

vec3 debugthing(vec3 light_pos, vec4 light_color, vec3 pos, vec3 view,
                vec3 albedo, vec3 normal, float metallic, float roughness)
{
	return albedo*0.5;
}

void main(void) {
	uint cluster = CURRENT_CLUSTER();

	vec4 texcolor = texture2D(diffuse_map, f_texcoord);
	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	vec3 metal_roughness_idx = texture2D(specular_map, f_texcoord).rgb;
	vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));
	vec4 radmap = textureCubeAtlas(irradiance_atlas, irradiance_probe, normal_dir);

	texcolor.rgb = pow(texcolor.rgb, vec3(2.2));

	vec3 albedo = f_color.rgb
		* anmaterial.diffuse.rgb
		* texcolor.rgb
		* anmaterial.diffuse.w * f_color.w;
	float opacity = texcolor.a * anmaterial.opacity;

	float metallic = anmaterial.metalness * metal_roughness_idx.b;
	float roughness = anmaterial.roughness * metal_roughness_idx.g;
	//float metallic = 0.0;
	//float roughness = 1.0;

	vec3 total_light = radmap.rgb * albedo;
	vec3 view_pos = vec3(v_inv * vec4(0, 0, 0, 1));
	vec3 view_dir = normalize(view_pos - f_position.xyz);
	mat4 mvp = p*v*m;

	LIGHT_LOOP(cluster, f_position.xyz, view_dir, albedo,
	           normal_dir, metallic, roughness, 1.0);

#if DEBUG_CLUSTERS
	float N = float(ACTIVE_POINTS(cluster)) / float(MAX_LIGHTS);
	const float thresh = 0.75;
	const float invthresh = thresh / 1.0;
	FRAG_COLOR = vec4(total_light +
		vec3(invthresh * max(0.0, N - thresh), 1.0 - N, N), 1.0);
#else
	FRAG_COLOR = vec4(total_light, opacity);
#endif
}
