#version 100
precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

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
	float shininess;
	float opacity;
};

// TODO: "active lights" count, pack lights[] tightly
const int max_lights = 32;
uniform lightSource lights[max_lights];
uniform material anmaterial;
uniform vec4 lightpos;

void main(void) {
	// TODO: this flips the normals too...
	vec2 flipped_texcoord = vec2(f_texcoord.x, 1.0 - f_texcoord.y);

	//vec2 flipped_texcoord = f_texcoord;
	vec3 normidx = texture2D(normal_map, flipped_texcoord).rgb;

	vec3 ambient_light = vec3(0.2);
	//vec3 normal_dir = normalize(f_normal);
	//mat3 TBN = transpose(mat3(f_tangent, f_bitangent, f_normal));
	//vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));
	// flipped_texcoord also flips normal maps, so multiply the y component
	// by -1 here to flip it
	vec3 normal_dir = vec3(1, -1, 1) * normalize(normidx * 2.0 - 1.0);
	normal_dir = normalize(TBN * normal_dir);

	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	mat4 mvp = p*v*m;
	vec3 total_light = vec3(anmaterial.diffuse.x * ambient_light.x,
	                        anmaterial.diffuse.y * ambient_light.y,
	                        anmaterial.diffuse.z * ambient_light.z);

	for (int i = 0; i < max_lights; i++) {
		if (!lights[i].is_active) {
			continue;
		}

		vec3 light_dir;
		float attenuation;

		vec3 light_vertex = vec3(lights[i].position - f_position);
		float distance = length(light_vertex);
		attenuation = 1.0 / (lights[i].const_attenuation
							 + lights[i].linear_attenuation * distance
							 + lights[i].quadratic_attenuation * distance * distance);

		attenuation = mix(1.0, attenuation, lights[i].position.w);
		light_dir = normalize(light_vertex / distance);

		vec3 diffuse_reflection = vec3(0.0);
		vec3 specular_reflection = vec3(0.0);
		vec3 environment_reflection = vec3(0.0);
		vec3 environment_refraction = vec3(0.0);

#if ENABLE_DIFFUSION
		float aoidx = texture2D(ambient_occ_map, flipped_texcoord).r;
		diffuse_reflection =
			// diminish contribution from diffuse lighting for a more cartoony look
			//0.5 *
			aoidx * anmaterial.diffuse.w *
			attenuation * vec3(lights[i].diffuse) * vec3(anmaterial.diffuse)
			* max(0.0, dot(normal_dir, light_dir));
#endif

#if ENABLE_SPECULAR_HIGHLIGHTS
		float specidx = texture2D(specular_map, flipped_texcoord).r;

		if (anmaterial.shininess > 0.1 && dot(normal_dir, light_dir) >= 0.9) {
			specular_reflection = anmaterial.specular.w * attenuation
				* vec3(lights[i].specular)
				* vec3(anmaterial.specular)
				* pow(max(0.0, dot(reflect(-light_dir, normal_dir), view_dir)),
				      anmaterial.shininess * (length(specidx) + 1.0));
		}

		specular_reflection *= specidx;
		//specular_reflection *= specidx;
#endif

#if ENABLE_SKYBOX
		vec3 env_light
			= vec3(textureCube(skytexture, reflect(-view_dir, normal_dir)))
			* anmaterial.shininess/1000.0 * specidx;
			//* 0.1
			;
#endif

#if ENABLE_REFRACTION
		vec3 ref_light = vec3(0);

		if (anmaterial.opacity < 1.0) {
			ref_light = anmaterial.diffuse.xyz
			* 0.5*vec3(textureCube(skytexture,
			                       refract(-view_dir, normal_dir, 1.0/1.5)));
		}
#endif

		//total_light += diffuse_reflection + specular_reflection + env_light;
		total_light +=
			mix(ref_light, diffuse_reflection, anmaterial.opacity)
			+ mix(specular_reflection, env_light, 0.5)
			;
	}

	vec4 dispnorm = vec4((normal_dir + 1.0)/2.0, 1.0);

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
