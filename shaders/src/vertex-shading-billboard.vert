#define VERTEX_SHADER

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/attenuation.glsl>
#include <lib/shading-varying.glsl>
#include <lib/instanced-uniforms.glsl>
#include <lib/tonemapping.glsl>

//#define LIGHT_FUNCTION blinn_phong_lighting
//#include <lighting/blinn-phong.glsl>
#define LIGHT_FUNCTION mrp_lighting
#define MRP_USE_SCHLICK_GGX
#define MRP_USE_LAMBERT_DIFFUSE
#include <lighting/metal-roughness-pbr.glsl>
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
	//vec3 normal_dir = normalize(m_3x3_inv_transp * v_normal);
	//vec3 normal_dir = v_normal;

	f_normal = v_normal;
	f_tangent = v_tangent;
	f_bitangent = vec4(cross(v_normal, v_tangent.xyz) * v_tangent.w, v_tangent.w);

	vec3 T = normalize(vec3(transforms[gl_InstanceID] * m * v_tangent));
	vec3 B = normalize(vec3(transforms[gl_InstanceID] * m * f_bitangent));
	vec3 N = normalize(vec3(transforms[gl_InstanceID] * m * vec4(v_normal, 0)));

	TBN = mat3(T, B, N);

	// TODO: make this a uniform
	vec3 normal_dir = N;
	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1)) - position);

	vec3 albedo = v_color.rgb * anmaterial.diffuse.rgb;

	vec3 Fspec = F(f_0(albedo, anmaterial.metalness), view_dir, normalize(view_dir + normal_dir));
	vec3 Fdiff = 1.0 - Fspec;
	vec4 radmap = textureCubeAtlas(irradiance_atlas, irradiance_probe, normal_dir);
	vec3 total_light = radmap.xyz * Fdiff;

	// TODO: no shadow loop
	LIGHT_LOOP(cluster, position, view_dir, vec3(1.0),
	           normal_dir, anmaterial.metalness, anmaterial.roughness, 1.0);

	ex_Color = total_light;

	f_position = transforms[gl_InstanceID] * m * vec4(in_Position, 1);
	f_texcoord = texcoord;
	f_lightmap = a_lightmap;
	f_color    = v_color;

	gl_Position = p*v * f_position;
}
