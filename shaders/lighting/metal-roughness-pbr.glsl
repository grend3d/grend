#pragma once
#include <lib/shading-uniforms.glsl>

// values and formulas from khronos gltf PBR implementation
const vec3 dielectric_specular = vec3(0.04);
const vec3 black = vec3(0);

float nzdot(vec3 a, vec3 b) {
	float d = dot(a, b);

	return (d < 0.0)? min(-0.001, d) : max(0.001, d);
}

float posdot(vec3 a, vec3 b) {
	return max(0.001, dot(a, b));
}

float mindot(vec3 a, vec3 b) {
	return max(0.0, dot(a, b));
}

float absdot(vec3 a, vec3 b) {
	return abs(dot(a, b));
}

vec3 c_diff(vec3 base_color, float metallic) {
	return mix(base_color*(1.0 - dielectric_specular[0]), black, metallic);
}

vec3 f_0(vec3 base_color, float metallic) {
	return mix(dielectric_specular, base_color, metallic);
}

float alpha(float roughness) {
	return roughness*roughness;
}

// fresnel schlick
vec3 F(vec3 f_0, vec3 V, vec3 H) {
	// no not posdot
	return f_0 + (1.0 - f_0) * pow(1.0 - dot(V, H), 5.0);
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
	vec3 v2 = vec3(2.0);

	float foo = 0.5
		/ ((posdot(N, L) * sqrt(pow(dot(N, V), 2.0) * (1.0 - pow(a, 2.0)) + pow(a, 2.0)))
		+  (posdot(N, V) * sqrt((pow(dot(N, L), 2.0)) * (1.0 - pow(a, 2.0)) + pow(a, 2.0))));

	return foo;
}
#endif

// trowbridge-reitz microfacet distribution
float D(float a, vec3 N, vec3 H) {
	return pow(a, 2.0)
		/ (PI * pow(pow(dot(N, H), 2.0) * (pow(a, 2.0) - 1.0) + 1.0, 2.0));
}

vec3 f_specular(vec3 F, float vis, float D) {
	return F*vis*D;
}

vec3 f_thing(vec3 spec, vec3 N, vec3 L, vec3 V) {
	return spec / max(0.25, (4.0 * nzdot(N, -L) * nzdot(N, V)));
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

	vec3 light_dir = normalize(light_vertex / dist);
	vec3 half_dir = normalize(light_dir + view);

	vec3 base = anmaterial.diffuse.w * light_color.w
		* vec3(light_color)*vec3(anmaterial.diffuse)
		* albedo;

	vec3 Fa = F(f_0(base, metallic), view, half_dir);
	vec3 ca = c_diff(base, metallic);
	vec3 fa_diff = f_diffuse(Fa, c_diff(base, metallic));
	vec3 fa_spec = f_specular(Fa, G(a, normal, light_dir, view),
			D(a, normal, half_dir))
			* vec3(light_color) * light_color.w;
	fa_spec = f_thing(fa_spec, normal, light_dir, view);

	return PI*max(0.0, dot(normal, light_dir)) * (fa_diff+fa_spec);
	//return fa_diff + 0.5*fa_spec;
}
