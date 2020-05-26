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
#include <lib/tonemapping.glsl>
#include <lighting/metal-roughness-pbr.glsl>

void main(void) {
	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	mat4 mvp = p*v*m;

	vec3 albedo = texture2D(diffuse_map, f_texcoord).rgb;
	float metallic = anmaterial.metalness;
	float roughness = anmaterial.roughness;
	vec3 total_light = vec3(0);

	for (int i = 0; i < active_point_lights; i++) {
		float atten = point_attenuation(i, vec3(f_position));
		vec3 lum = mrp_lighting(point_lights[i].position, point_lights[i].diffuse, 
		                        vec3(f_position), view_dir,
		                        albedo, f_normal, metallic, roughness);

		total_light += lum*atten;
	}

	for (int i = 0; i < active_spot_lights; i++) {
		float atten = spot_attenuation(i, vec3(f_position));
		vec3 lum = mrp_lighting(spot_lights[i].position, spot_lights[i].diffuse, 
		                        vec3(f_position), view_dir,
		                        albedo, f_normal, metallic, roughness);

		total_light += lum*atten;
	}

	for (int i = 0; i < active_directional_lights; i++) {
		float atten = directional_attenuation(i, vec3(f_position));
		vec3 lum = mrp_lighting(directional_lights[i].position,
		                        directional_lights[i].diffuse, 
		                        vec3(f_position), view_dir,
		                        albedo, f_normal, metallic, roughness);

		total_light += lum*atten;
	}

	FRAG_COLOR = vec4(total_light, 1.0);
}
