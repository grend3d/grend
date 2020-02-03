#version 150
precision highp float;

varying vec3 f_normal;
varying vec3 f_tangent;
varying vec3 f_bitangent;
varying vec2 f_texcoord;
varying vec4 f_position;
varying mat3 TBN;

// light maps
uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform sampler2D ambient_occ_map;

uniform mat4 m, v, p;
uniform mat3 m_3x3_inv_transp;
uniform mat4 v_inv;

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1

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
                                       vec4(0), vec4(1.0), 5);

uniform vec4 lightpos;

void main(void) {
	// TODO: this flips the normals too...
	vec2 flipped_texcoord = vec2(f_texcoord.x, 1.0 - f_texcoord.y);

	//vec2 flipped_texcoord = f_texcoord;
	vec3 normidx = texture2D(normal_map, flipped_texcoord).rgb;

	vec3 ambient_light = vec3(0.0);
	//vec3 normal_dir = normalize(f_normal);
	//mat3 TBN = transpose(mat3(f_tangent, f_bitangent, f_normal));
	//vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));
	// flipped_texcoord also flips normal maps, so multiply the y component
	// by -1 here to flip it
	vec3 normal_dir = vec3(1, -1, 1) * normalize(normidx * 2.0 - 1.0);
	normal_dir = normalize(TBN * normal_dir);

	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));

	lights[0] = lightSource(lightpos,
	                        vec4(0.8, 0.8, 0.7, 1.0),
	                        //vec4(0.0, 0.0, 0.0, 0),
	                        1, 0.0, 0.005,
	                        1);

	lights[1] = lightSource(vec4(-8.0, 4.0, -8.0, 0.5),
	                        vec4(1, 0.7, 0.4, 1.0),
	                        //vec4(0.0, 0.0, 0.0, 0),
	                        1, 0.0, 0.02,
	                        1);

	lights[2] = lightSource(vec4(-8,  4.0, 8.0, 0.5),
	                        vec4(1.0, 1.0, 0.9, 1.0),
	                        //vec4(0.0, 0.0, 0.0, 0),
	                        1, 0.0, 0.05,
	                        1);

	lights[3] = lightSource(vec4( 8, 4.0, 8.0, 0.2),
	                        vec4(1, 0.5, 0, 1.0),
	                        //1, 0.00, 0.02);
	                        0.75, 0.0, 0.05,
	                        1);

	mat4 mvp = p*v*m;
	vec3 total_light = vec3(anmaterial.diffuse.x * ambient_light.x,
	                        anmaterial.diffuse.y * ambient_light.y,
	                        anmaterial.diffuse.z * ambient_light.z);

	for (int i = 0; i < light_count; i++) {
		vec3 light_dir;
		float attenuation;

		vec3 light_vertex = vec3(lights[i].position - f_position);
		float distance = length(light_vertex);
		attenuation = 1.0 / (lights[i].const_attenuation
							 + lights[i].linear_attenuation * distance
							 + lights[i].quadratic_attenuation * distance * distance);

		attenuation = mix(1, attenuation, lights[i].position.w);
		light_dir = normalize(light_vertex / distance);

		vec3 diffuse_reflection = vec3(0.0);
		vec3 specular_reflection = vec3(0);

#if ENABLE_DIFFUSION
		vec3 aoidx = texture2D(ambient_occ_map, flipped_texcoord).rgb;
		diffuse_reflection =
			// diminish contribution from diffuse lighting for a more cartoony look
			//0.5 *
			aoidx *
			attenuation * vec3(lights[i].diffuse) * vec3(anmaterial.diffuse)
			* max(0.0, dot(normal_dir, light_dir));
#endif

#if ENABLE_SPECULAR_HIGHLIGHTS
		vec3 specidx = texture2D(specular_map, flipped_texcoord).rgb;

		if (anmaterial.shininess > 0.1 && dot(normal_dir, light_dir) >= 0) {
			specular_reflection = anmaterial.specular.w * attenuation
				* vec3(lights[i].specular)
				* vec3(anmaterial.specular)
				* pow(max(0.0, dot(reflect(-light_dir, normal_dir), view_dir)),
				      anmaterial.shininess * (length(specidx) + 1));
		}

		specular_reflection *= specidx;
		//specular_reflection *= specidx;
#endif

		total_light += diffuse_reflection + specular_reflection;
	}

	vec4 dispnorm = vec4((normal_dir + 1)/2, 1);

	//gl_FragColor = vec4(total_light, 1.0) * mix(texture2D(diffuse_map, flipped_texcoord), texture2D(normal_map, flipped_texcoord), 0.5) * mix(vec4(1.0), unused, 0.00001);
	//gl_FragColor = vec4(total_light, 1.0) * texture2D(diffuse_map, flipped_texcoord);
	//gl_FragColor = vec4((normal_dir+1)/2, 1) + unused;

	vec4 displight = vec4(total_light, 1.0) * texture2D(diffuse_map, flipped_texcoord);
	//vec4 displight = vec4(total_light, 1.0);

	//gl_FragColor = mix(dispnorm, displight, 0.25);
	//gl_FragColor = mix(dispnorm, displight, 0.5);
	//gl_FragColor = mix(dispnorm, displight, 0.99);
	gl_FragColor = displight;
}
