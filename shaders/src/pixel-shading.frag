#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#define ENABLE_DIFFUSION 1
#define ENABLE_SPECULAR_HIGHLIGHTS 1
#define ENABLE_SKYBOX 1
#define ENABLE_REFRACTION 1

#include <lib/compat.glsl>
#include <lib/shading-uniforms.glsl>
#include <lib/shading-varying.glsl>
#include <lib/attenuation.glsl>

void main(void) {
	vec3 normidx = texture2D(normal_map, f_texcoord).rgb;

	vec3 ambient_light = vec3(0.0);
	//vec3 normal_dir = normalize(f_normal);
	//mat3 TBN = transpose(mat3(f_tangent, f_bitangent, f_normal));
	//vec3 normal_dir = normalize(TBN * normalize(normidx * 2.0 - 1.0));
	// f_texcoord also flips normal maps, so multiply the y component
	// by -1 here to flip it
	vec3 normal_dir = vec3(1, -1, 1) * normalize(normidx * 2.0 - 1.0);
	normal_dir = normalize(TBN * normal_dir);

	vec3 view_dir = normalize(vec3(v_inv * vec4(0, 0, 0, 1) - f_position));
	mat4 mvp = p*v*m;
	vec3 total_light = vec3(anmaterial.diffuse.x * ambient_light.x,
	                        anmaterial.diffuse.y * ambient_light.y,
	                        anmaterial.diffuse.z * ambient_light.z);

	for (int i = 0; i < active_lights && i < max_lights; i++) {
		vec3 light_dir;
		float lum = attenuation(i, f_position);

		vec3 light_vertex = vec3(lights[i].position - f_position);
		float distance = length(light_vertex);
		light_dir = normalize(light_vertex / distance);

		vec3 diffuse_reflection = vec3(0.0);
		vec3 specular_reflection = vec3(0.0);
		vec3 environment_reflection = vec3(0.0);
		vec3 environment_refraction = vec3(0.0);

#if ENABLE_DIFFUSION
		float aoidx = texture2D(ambient_occ_map, f_texcoord).r;
		diffuse_reflection =
			// diminish contribution from diffuse lighting for a more cartoony look
			//0.5 *
			aoidx * anmaterial.diffuse.w *
			lum * vec3(lights[i].diffuse) * vec3(anmaterial.diffuse)
			* max(0.0, dot(normal_dir, light_dir));
#endif

#if ENABLE_SPECULAR_HIGHLIGHTS
		float specidx = texture2D(specular_map, f_texcoord).r;

		if (anmaterial.metalness > 0.1 && dot(normal_dir, light_dir) >= 0.9) {
			specular_reflection = anmaterial.specular.w * lum
				* vec3(anmaterial.specular)
				* pow(max(0.0, dot(reflect(-light_dir, normal_dir), view_dir)),
				      anmaterial.metalness * (length(specidx) + 1.0));
		}

		specular_reflection *= specidx;
		//specular_reflection *= specidx;
#endif

#if ENABLE_SKYBOX
		vec3 env_light
			= vec3(textureCube(skytexture, reflect(-view_dir, normal_dir)))
			* anmaterial.metalness/1000.0 * specidx;
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

	//gl_FragColor = vec4(total_light, 1.0) * mix(texture2D(diffuse_map, f_texcoord), texture2D(normal_map, f_texcoord), 0.5) * mix(vec4(1.0), unused, 0.00001);
	//gl_FragColor = vec4(total_light, 1.0) * texture2D(diffuse_map, f_texcoord);
	//gl_FragColor = vec4((normal_dir+1)/2, 1) + unused;

	vec4 displight = vec4(total_light, 1.0) * texture2D(diffuse_map, f_texcoord);
	//vec4 displight = vec4(total_light, 1.0);

	//gl_FragColor = mix(dispnorm, displight, 0.25);
	//gl_FragColor = mix(dispnorm, displight, 0.5);
	//gl_FragColor = mix(dispnorm, displight, 0.99);
	gl_FragColor = displight;
}
