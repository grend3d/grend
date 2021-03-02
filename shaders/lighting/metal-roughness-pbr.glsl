#pragma once
//#include <lib/shading-uniforms.glsl>
#include <lib/utility.glsl>
#include <lib/constants.glsl>

#include <lighting/lambert-diffuse.glsl>
#include <lighting/oren-nayar-diffuse.glsl>
#include <lighting/fresnel-schlick.glsl>

// values and formulas from khronos gltf PBR implementation
const vec3 dielectric_specular = vec3(0.04);
const vec3 black = vec3(0);

vec3 c_diff(vec3 base_color, float metallic) {
	return mix(base_color*(1.0 - dielectric_specular[0]), black, metallic);
}

vec3 f_0(vec3 base_color, float metallic) {
	return mix(dielectric_specular, base_color, metallic);
}

float alpha(float roughness) {
	return roughness*roughness;
}

float pos(float x) {
	return max(0.001, x);
}

float nonzero(float x) {
	return (x == 0.0)? 0.001 : x;
}

#ifdef MRP_USE_SCHLICK_GGX
// schlick ggx
float G(float a, float NdotL, float NdotV) {
	float c = (a + 1.0);
	float k = c*c;

	return NdotV / (NdotV*(1.0 - k) + k);
	//return dot(N, V) / (nzdot(N, V)*(1.0 - k) + k);
}

#else
// smith joint ggx (default)
float G(float a, float NdotL, float NdotV) {
	float a2 = a*a;

	float foo = 0.5
		/ ((pos(NdotL) * sqrt(NdotV*NdotV * (1.0 - a2) + a2))
		+  (pos(NdotV) * sqrt(NdotL*NdotL * (1.0 - a2) + a2)));

/*
	float foo = 0.5
		/ ((posdot(N, L) * sqrt(pow(dot(N, V), 2.0) * (1.0 - a2) + a2))
		+  (posdot(N, V) * sqrt(pow(dot(N, L), 2.0) * (1.0 - a2) + a2)));
*/

	return foo;
}
#endif

// trowbridge-reitz microfacet distribution
float D(float a, float NdotH) {
	float denom = NdotH*NdotH * (a - 1.0) + 1.0;

	return a / (PI * denom * denom);
/*
	return a2
		/ (PI * pow(pow(dot(N, H), 2.0) * (a2 - 1.0) + 1.0, 2.0));
		*/
}

vec3 f_specular(vec3 F, float vis, float D) {
	return F*vis*D;
}

vec3 f_thing(vec3 spec, float NdotL, float NdotV) {
	//return spec / max(2.0, (4.0 * nzdot(N, L) * nzdot(N, V)));
	//return spec / max(0.01, (4.0 * nonzero(NdotL) * nonzero(NdotV)));
	return spec / max(1.0, (4.0 * nonzero(NdotL) * nonzero(NdotV)));
}

vec3 f_diffuse(vec3 F, vec3 c_diff) {
	return (1.0 - F) * (c_diff / PI);
}

vec3 mrp_lighting(vec3 light_pos, vec4 light_color, vec3 pos, vec3 view,
                  vec3 albedo, vec3 normal, float metallic, float roughness)
{
	float a = alpha(roughness);
	vec3 light_vertex = vec3(light_pos - pos);
	float dist = length(light_vertex);

	vec3 light_dir = light_vertex / dist;
	vec3 half_dir = normalize(light_dir + view);

	float NdotL = dot(normal, light_dir);
	float NdotV = dot(normal, view);
	float NdotH = dot(normal, half_dir);

	vec3 base = vec3(light_color) * albedo;

	vec3 Fa = F(f_0(base, metallic), view, half_dir);
	vec3 ca = c_diff(base, metallic);
	vec3 fa_diff = f_diffuse(Fa, c_diff(base, metallic));
	vec3 fa_spec = light_color.rgb
		* f_specular(Fa, G(a, NdotL, NdotV), D(a, NdotH));
	fa_spec = f_thing(fa_spec, NdotL, NdotV);

#ifdef MRP_USE_LAMBERT_DIFFUSE
	//return lambert_diffuse(a, light_dir, normal, view)*(fa_diff+fa_spec);
	return max(NdotL, 0.0) * (fa_diff + fa_spec);
#else
	return oren_nayar_diffuse(a, light_dir, normal, view)*(fa_diff + fa_spec);
#endif
}
