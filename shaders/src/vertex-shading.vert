#define VERTEX_SHADER

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/attenuation.glsl>
#include <lib/shading-varying.glsl>
#include <lib/tonemapping.glsl>

#define LIGHT_FUNCTION blinn_phong_lighting
#include <lighting/blinn-phong.glsl>
#include <lighting/lightingLoop.glsl>

OUT vec3 ex_Color;

vec3 get_position(mat4 m, vec4 coord) {
	vec4 temp = m*coord;
	return temp.xyz / temp.w;
}

void main(void) {
	// TODO: clusters sort of don't make sense for vertex shading, since a vertex
	//       could lie outside of a a cluster but have fragments that lie inside...
	//       Possibly just iterate over all lights?
	uint cluster = CURRENT_CLUSTER();
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
	float shininess = 1.0 - anmaterial.roughness * anmaterial.roughness;

	// TODO: no shadow loop
	LIGHT_LOOP(cluster, position, view_dir, anmaterial.diffuse.xyz,
	           normal_dir, anmaterial.metalness, roughness, 1.0);

	ex_Color = EARLY_TONEMAP(vec3(total_light), 1.0); 
	f_texcoord = texcoord;
	gl_Position = mvp * vec4(in_Position, 1.0);
	//gl_Position = mvp * vec4(in_Position, 0.25);
}
