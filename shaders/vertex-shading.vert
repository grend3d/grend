#version 150

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1

attribute vec2 texcoord;
varying vec2 f_texcoord;

attribute vec3 v_normal;
uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;;

in vec3 in_Position;
in vec3 in_Color;
out vec3 ex_Color;

struct lightSource {
	vec4 position;
	vec4 diffuse;
	float const_attenuation, linear_attenuation, quadratic_attenuation;
	float specular;
};

struct material {
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	float shininess;
};

const int light_count = 4;
lightSource lights[light_count];

uniform material anmaterial = material(vec4(1.0, 1.0, 1.0, 1.0),
                               vec4(0),
                               vec4(1.0),
                               5);

void main(void) {
	vec4 v_coord = vec4(in_Position, 1.0);
	vec3 ambient_light = vec3(1);
	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - m * v_coord));

	lights[0] = lightSource(vec4(8.0, 16.0, -8.0, 1.0),
	                        vec4(0.8, 0.8, 0.7, 1.0),
	                        //vec4(0.0, 0.0, 0.0, 0),
	                        1, 0.0, 0.002,
	                        1);

	lights[1] = lightSource(vec4(-8.0, 16.0, -8.0, 1.0),
	                        vec4(1, 0.7, 0.4, 1.0),
	                        //vec4(0.0, 0.0, 0.0, 0),
	                        1, 0.0, 0.002,
	                        1);

	lights[2] = lightSource(vec4(-8, 16.0, 8.0, 1.0),
	                        vec4(1.0, 1.0, 0.9, 1.0),
	                        //vec4(0.0, 0.0, 0.0, 0),
	                        1, 0.0, 0.005,
	                        1);

	lights[3] = lightSource(vec4( 8, 16.0, 8.0, 1.0),
	                        vec4(1, 0.5, 0, 1.0),
	                        //1, 0.00, 0.02);
	                        0.75, 0.0, 0.05,
	                        1);

	mat4 mvp = p*v*m;
	//vec3 total_light = ambient_light;
	vec3 total_light = vec3(anmaterial.diffuse.x * ambient_light.x,
	                        anmaterial.diffuse.y * ambient_light.y,
	                        anmaterial.diffuse.z * ambient_light.z);
	vec3 normal_dir = normalize(m_3x3_inv_transp * v_normal);

	for (int i = 0; i < light_count; i++) {
		vec3 light_dir;
		float attenuation;

		if (lights[i].position.w == 0.0) {
			attenuation = 1;
			light_dir = normalize(vec3(lights[i].position));

		} else {
			vec3 light_vertex = vec3(lights[i].position - m * v_coord);
			float distance = length(light_vertex);
			attenuation = 1.0 / (lights[i].const_attenuation
			                     + lights[i].linear_attenuation * distance
			                     + lights[i].quadratic_attenuation * distance * distance);

			//attenuation = mix(1, attenuation, lights[i].position.w)
			light_dir = normalize(light_vertex);
			//light_dir = normalize(vec3(lights[i].position));
		}

		vec3 diffuse_reflection = vec3(0.0);
		vec3 specular_reflection = vec3(0);

#if ENABLE_DIFFUSION
		diffuse_reflection =
			// diminish contribution from diffuse lighting for a more cartoony look
			0.5 *
			attenuation * vec3(lights[i].diffuse) * vec3(anmaterial.diffuse)
			* max(0.0, dot(normal_dir, light_dir));
#endif

#if ENABLE_SPECULAR_HIGHLIGHTS
		if (anmaterial.shininess > 0.1 && dot(normal_dir, light_dir) >= 0) {
			specular_reflection = attenuation * vec3(lights[i].specular)
				* vec3(anmaterial.specular)
				* pow(max(0.0, dot(reflect(-light_dir, normal_dir), view_dir)),
				      anmaterial.shininess);
		}
#endif

		total_light += diffuse_reflection + specular_reflection;
	}

	ex_Color = vec3(total_light); 
	gl_Position = normalize(mvp * vec4(in_Position, 1.0));
	//gl_Position = mvp * vec4(in_Position, 0.25);

	f_texcoord = texcoord;
}
