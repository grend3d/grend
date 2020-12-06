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

#ifdef MRP_USE_SCHLICK_GGX
// schlick ggx
float G(float a, vec3 N, vec3 L, vec3 V) {
	float k = pow(a + 1.0, 2.0);

	return dot(N, V) / (nzdot(N, V)*(1.0 - k) + k);
}

#else
// smith joint ggx (default)
float G(float a, vec3 N, vec3 L, vec3 V) {
	float a2 = a*a;

	float foo = 0.5
		/ ((posdot(N, L) * sqrt(pow(dot(N, V), 2.0) * (1.0 - a2) + a2))
		+  (posdot(N, V) * sqrt(pow(dot(N, L), 2.0) * (1.0 - a2) + a2)));
		/*
	float foo = 0.5
		/ ((max(a+0.0001, dot(N, L)) * sqrt(pow(dot(N, V), 2.0) * (1.0 - a2) + a2))
		+  (max(a+0.0001, dot(N, V)) * sqrt(pow(dot(N, L), 2.0) * (1.0 - a2) + a2)));
		*/

	return foo;
}
#endif

// trowbridge-reitz microfacet distribution
float D(float a, vec3 N, vec3 H) {
	float a2 = a*a;
	return a2
		/ (PI * pow(pow(dot(N, H), 2.0) * (a2 - 1.0) + 1.0, 2.0));
}

vec3 f_specular(vec3 F, float vis, float D) {
	return F*vis*D;
}

vec3 f_thing(vec3 spec, vec3 N, vec3 L, vec3 V) {
	return spec / max(2.0, (4.0 * nzdot(N, L) * nzdot(N, V)));
}

vec3 f_diffuse(vec3 F, vec3 c_diff) {
	return max(vec3(0), (1.0 - F) * (c_diff / PI));
}

vec3 mrp_lighting(vec3 light_pos, vec4 light_color, vec3 pos, vec3 view,
                  vec3 albedo, vec3 normal, float metallic, float roughness)
{
	float a = alpha(roughness);
	vec3 light_vertex = vec3(light_pos - pos);
	float dist = length(light_vertex);

	vec3 light_dir = normalize(light_vertex / dist);
	vec3 half_dir = normalize(light_dir + view);

	vec3 base = light_color.w * vec3(light_color) * albedo;

	vec3 Fa = F(f_0(base, metallic), view, half_dir);
	vec3 ca = c_diff(base, metallic);
	vec3 fa_diff = f_diffuse(Fa, c_diff(base, metallic));
	vec3 fa_spec = f_specular(Fa, G(a, normal, light_dir, view),
			D(a, normal, half_dir))
			* vec3(light_color) * light_color.w;
	fa_spec = f_thing(fa_spec, normal, light_dir, view);

	// TODO; configurable
	//return PI*max(0.0, dot(normal, light_dir)) * (fa_diff+fa_spec);
	//return fa_diff + 0.5*fa_spec;
#ifdef MRP_USE_LAMBERT_DIFFUSE
	return lambert_diffuse(roughness, light_dir, normal, view)*(fa_diff+fa_spec);
#else
	return oren_nayar_diffuse(roughness, light_dir, normal, view)*(fa_diff + fa_spec);
#endif
}
