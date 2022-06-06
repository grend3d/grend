#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#define LIGHT_FUNCTION mrp_lighting

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>

#include <lib/shadows.glsl>
#include <lib/attenuation.glsl>
#include <lib/tonemapping.glsl>
#include <lighting/metal-roughness-pbr.glsl>
#include <lighting/lightingLoop.glsl>

void main(void) {
	uint cluster = SCREEN_TO_CLUSTER(gl_FragCoord.x/screen_x, gl_FragCoord.y/screen_y);
	// TODO:
	//vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;

	vec4  texcolor            = texture2D(render_fb, f_texcoord);
	// TODO: emissive
	//vec3  emissive            = texture2D(, f_texcoord).rgb;
	vec3  normal              = texture2D(normal_fb, f_texcoord).rgb;
	vec3  position            = texture2D(position_fb, f_texcoord).rgb;
	vec3  metal_roughness_idx = texture2D(metalroughness_fb, f_texcoord).rgb;

	vec3 albedo = texcolor.rgb;
	float metallic  = metal_roughness_idx.r;
	float roughness = metal_roughness_idx.g;
	// TODO: store AO in metal-roughness buffer
	float aoidx     = 1.0;

	vec3 view_pos = vec3(v_inv * vec4(0, 0, 0, 1));
	vec3 view_vec = view_pos - position;
	vec3 view_dir = normalize(view_vec);

	// TODO: probes
	vec3 total_light = vec3(0);
	LIGHT_LOOP(cluster,
	           position,
	           view_dir,
	           albedo,
	           normal,
	           metallic,
	           roughness,
	           aoidx);

	//FRAG_COLOR = doTonemap(vec4(total_light, 1.0), exposure);
	FRAG_COLOR = vec4(total_light, 1.0);
}
