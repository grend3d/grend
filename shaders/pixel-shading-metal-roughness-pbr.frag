#version 100
precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define PI 3.1415926

varying vec3 f_normal;
varying vec3 f_tangent;
varying vec3 f_bitangent;
varying vec2 f_texcoord;
varying vec4 f_position;
varying mat3 TBN;

// light maps
uniform sampler2D diffuse_map;
// TODO: this is the metal-roughness map, need to rename things
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D ambient_occ_map;
uniform sampler2D alpha_map;
uniform samplerCube skytexture;

uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1
#define ENABLE_REFRACTION 1

struct lightSource {
	vec4 position;
	vec4 diffuse;
	float const_attenuation, linear_attenuation, quadratic_attenuation;
	float specular;
	bool is_active;
};

struct material {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	float roughness;
	float metalness;
	float opacity;
};

// TODO: "active lights" count, pack lights[] tightly
const int max_lights = 32;
uniform lightSource lights[max_lights];
uniform material anmaterial;
uniform vec4 lightpos;

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

#if 1
// smith joint ggx
float G(float a, vec3 N, vec3 L, vec3 V) {
	vec3 v2 = vec3(2.0);

	float foo = 0.5
		/ ((posdot(N, L) * sqrt(pow(dot(N, V), 2.0) * (1.0 - pow(a, 2.0)) + pow(a, 2.0)))
		+  (posdot(N, V) * sqrt((pow(dot(N, L), 2.0)) * (1.0 - pow(a, 2.0)) + pow(a, 2.0))));

	return foo;
}

#else
// schlick ggx
float G(float a, vec3 N, vec3 L, vec3 V) {
	float k = pow(a + 1.0, 2.0);

	return dot(N, V) / (nzdot(N, V)*(1.0 - k) + k);
}
#endif

// trowbridge-reitz microfacet distribution
float D(float a, vec3 N, vec3 H) {
	return pow(a, 2.0)
		/ (PI * pow(pow(posdot(N, H), 2.0) * (pow(a, 2.0) - 1.0) + 1.0, 2.0));
}

vec3 f_specular(vec3 F, float vis, float D) {
	return F*vis*D;
}

vec3 f_thing(vec3 spec, vec3 N, vec3 L, vec3 V) {
	return spec / max(0.05, (4.0 * nzdot(N, -L) * nzdot(N, V)));
}

vec3 f_diffuse(vec3 F, vec3 c_diff) {
	return (1.0 - F) * (c_diff / PI);
}

void main(void) {
	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;
	float shininess = (1.0 - anmaterial.roughness) * 1000.0;

	vec3 ambient_light = vec3(0.2);
	// f_texcoord also flips normal maps, so multiply the y component
	// by -1 here to flip it
	vec3 normal_dir = vec3(1, -1, 1) * normalize(normidx * 2.0 - 1.0);
	normal_dir = normalize(TBN * normal_dir);

	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	mat4 mvp = p*v*m;

	float metallic = anmaterial.metalness * texture2D(specular_map, f_texcoord).b;
	float roughness = anmaterial.roughness * texture2D(specular_map, f_texcoord).g;
	float a = alpha(roughness);
	float aoidx = pow(texture2D(ambient_occ_map, f_texcoord).r, 2.0);

	vec3 total_light = vec3(0);

	for (int i = 0; i < max_lights; i++) {
	//for (int i = 0; i < 1; i++) {
		if (!lights[i].is_active) {
			continue;
		}

		vec3 light_vertex = vec3(lights[i].position - f_position);
		float dist = length(light_vertex);

		//vec3 light_dir = normalize(light_vertex / dist);
		vec3 light_dir = normalize(light_vertex);
		vec3 half_dir = normalize(light_dir + view_dir);
		//vec3 half_dir = vec3(1);

		float attenuation;
		attenuation = 1.0 / (lights[i].const_attenuation
							 + lights[i].linear_attenuation * dist
							 + lights[i].quadratic_attenuation * PI * dist * dist);
		//attenuation = mix(1.0, attenuation, lights[i].position.w);

		vec3 base = anmaterial.diffuse.w *
			vec3(lights[i].diffuse)*vec3(anmaterial.diffuse)
			* texture2D(diffuse_map, f_texcoord).rgb;

		vec3 Fa = F(f_0(base, metallic), view_dir, half_dir);
		vec3 ca = c_diff(base, metallic);
		vec3 fa_diff = f_diffuse(Fa, c_diff(base, metallic));
		vec3 fa_spec = f_specular(Fa, G(a, normal_dir, light_dir, view_dir),
			                  D(a, normal_dir, half_dir));
		fa_spec = f_thing(fa_spec, normal_dir, light_dir, view_dir);

		total_light += aoidx*attenuation*((fa_diff) + 0.5*fa_spec)
			+ (0.05*metallic*(1.0 - roughness))*(1.0 - fa_spec)*vec3(textureCube(skytexture, reflect(-view_dir, normal_dir)));
	}

#if ENABLE_REFRACTION
	vec3 ref_light = vec3(0);

	if (anmaterial.opacity < 1.0) {
		ref_light = anmaterial.diffuse.xyz
			* 0.5*vec3(textureCube(skytexture,
			                       refract(-view_dir, normal_dir, 1.0/1.5)));
	}

	total_light += ref_light;
#endif

	vec4 dispnorm = vec4((normal_dir + 1.0)/2.0, 1.0);
	vec4 displight = vec4(total_light, 1.0);
	gl_FragColor = displight;
}
