#define VERTEX_SHADER

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/attenuation.glsl>
#include <lib/shading-varying.glsl>
#include <lib/tonemapping.glsl>

in vec3 in_Position;
in vec2 texcoord;
in vec3 in_Color;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;

out vec3 ex_Color;

vec3 gouraud_lighting(vec3 light_pos, vec4 light_color, vec3 pos, vec3 view,
                      vec3 albedo, vec3 normal)
{
	vec3 light_dir;
	float dist = distance(light_pos, pos);
	light_dir = normalize(light_pos - pos);

	vec3 diffuse_reflection = vec3(0.0);
	vec3 specular_reflection = vec3(0);
	// TODO: once we have light probes and stuff
	vec3 env_light = vec3(0);

	diffuse_reflection =
		vec3(light_color) * vec3(anmaterial.diffuse)
		* max(0.0, dot(normal, light_dir));

	if (anmaterial.metalness > 0.1 && dot(normal, light_dir) >= 0.0) {
		specular_reflection = anmaterial.specular.w
			* vec3(anmaterial.specular)
			* pow(max(0.0, dot(reflect(-light_dir, normal), view)),
					anmaterial.metalness);
	}

	return diffuse_reflection + specular_reflection;
}

vec3 get_position(mat4 m, vec4 coord) {
	vec4 temp = m*coord;
	return temp.xyz / temp.w;
}

void main(void) {
	vec3 position = get_position(m, vec4(in_Position, 1.0));

	// TODO: make this a uniform
	vec3 ambient_light = vec3(0.0);
	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1)) - position);

	mat4 mvp = p*v*m;
	//vec3 total_light = ambient_light;
	vec3 total_light = vec3(anmaterial.diffuse.x * ambient_light.x,
	                        anmaterial.diffuse.y * ambient_light.y,
	                        anmaterial.diffuse.z * ambient_light.z);
	vec3 normal_dir = normalize(m_3x3_inv_transp * v_normal);

	for (int i = 0; i < max_lights; i++) {
		float lum = point_attenuation(i, position);

		total_light += lum *
			gouraud_lighting(point_lights[i].position, point_lights[i].diffuse,
			                 position, view_dir, anmaterial.diffuse.xyz, normal_dir);
	}

	for (int i = 0; i < max_lights; i++) {
		float lum = spot_attenuation(i, position);

		total_light += lum *
			gouraud_lighting(spot_lights[i].position, spot_lights[i].diffuse,
			                 position, view_dir, anmaterial.diffuse.xyz, normal_dir);
	}

	for (int i = 0; i < max_lights; i++) {
		float lum = directional_attenuation(i, position);

		total_light += lum *
			gouraud_lighting(directional_lights[i].position,
			                 directional_lights[i].diffuse,
			                 position, view_dir, anmaterial.diffuse.xyz, normal_dir);
	}

	ex_Color = EARLY_TONEMAP(vec3(total_light), 1.0); 
	gl_Position = normalize(mvp * vec4(in_Position, 1.0));
	//gl_Position = mvp * vec4(in_Position, 0.25);

	f_texcoord = texcoord;
}
