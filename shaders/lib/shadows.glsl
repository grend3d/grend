#pragma once
#include <lib/shading-uniforms.glsl>
#include <lib/atlas_cubemap.glsl>
#include <lib/davehash.glsl>

// TODO: make these uniforms
float near = 0.1;
float far = 50.0;

// TODO: compile-time and uniform parameter macros, once I get to
//       doing hot shader reloading
#define DEPTH_BIAS 0.00075
//#define SHADOW_FILTER shadow_sample // to disable shadow filtering
//#define SHADOW_FILTER shadow_sample_linear // linear interpolation only
//#define SHADOW_FILTER shadow_sample_noisy
#define SHADOW_FILTER shadow_pcf
//#define SHADOW_PCF_RANGE   0.5
#define SHADOW_PCF_RANGE   1.0
#define SHADOW_PCF_STEP    1.0

//#define SHADOW_PCF_SAMPLER shadow_sample_linear
#define SHADOW_PCF_SAMPLER shadow_sample_noisy
//#define SHADOW_PCF_SAMPLER shadow_sample

// possible parameter macro implementation:
// PARAMETER_UNIFORM_1F(name, default);
// => P_UNIFORM_1F(name,default)
//    which then gets pulled out while loading the shader
// PARAMETER_COMPILED_1F(name, default);
// PARAMETER_COMPILED_SYMBOL(name, default, choices ...);

// https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap
float vec_to_depth(vec3 vec) {
	vec3 ab = abs(vec);
	float z_comp = max(ab.x, max(ab.y, ab.z));
	float norm_z_comp = (far+near) / (far-near) - (2.0*far*near)/(far-near)/z_comp;

	return (norm_z_comp + 1.0) * 0.5;
}

float shadow_sample(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float depth = texture2DAtlas(atlas, slice, uv).r;

	return ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
}

float shadow_sample_noisy(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float pixsize = 3.0/texsize;

	vec3 noiz = hash32(1049.0*vert.xy + slice.xy + uv);
	//vec2 tuv = clamp(vec2(0), vec2(1), uv + (hash22(uv) - 0.5)*pixsize);
	vec2 tuv = uv + (noiz.xy - 0.5)*pixsize + 0.5/texsize;
	float depth = texture2DAtlas(atlas, slice, tuv).r;

	return ((depth + DEPTH_BIAS + noiz.z*DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
}

float shadow_sample_linear(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float pixsize = 1.0/texsize;
	vec2 part = fract(uv * texsize);

	#define SAMPLE(UV) shadow_sample(atlas, slice, vert, (UV))
	return
		mix(mix(SAMPLE(uv),                    SAMPLE(uv + vec2(0, pixsize)), part.y),
			mix(SAMPLE(uv + vec2(pixsize, 0)), SAMPLE(uv + pixsize),          part.y),
			part.x);
	#undef SAMPLE
}

#define PCF_FILTER(FNAME, RANGE, STEP, SAMPLER) \
	float FNAME(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) { \
		float ret = 0.0; \
		float texsize = float(textureSize(atlas, 0).x) * slice.z; \
		float off_factor = 1.0 / texsize; \
		\
		for (float x = -RANGE; x <= RANGE; x += STEP) { \
			for (float y = -RANGE; y <= RANGE; y += STEP) { \
				vec2 off = uv + vec2(x, y)*off_factor; \
				ret += SAMPLER(atlas, slice, vert, off); \
			} \
		} \
		\
		return ret/(4.0*RANGE*RANGE + 4.0*RANGE + 1.0); \
	}

PCF_FILTER(shadow_pcf, SHADOW_PCF_RANGE, SHADOW_PCF_STEP, SHADOW_PCF_SAMPLER)

// // 4 sample, interpolated (16 total samples)
// PCF_FILTER(shadow_pcf4_interp, 0.5,  1.0, shadow_sample_linear);
// // 9 sample, interpolated (36 total samples)
// PCF_FILTER(shadow_pcf9_interp, 1.0,  1.0, shadow_sample_linear);
// // 16 samples
// PCF_FILTER(shadow_pcf16,       1.5,  1.0, shadow_sample);
// // 64 samples
// PCF_FILTER(shadow_pcf64,       1.75, 0.5, shadow_sample);

float point_shadow(uint idx, vec3 pos) {
	// TODO: can branching be avoided without the texture sample?
	if (!POINT_LIGHT(idx).casts_shadows) {
		return 1.0;
	}

	vec3 light_vertex = POINT_LIGHT(idx).position.xyz - pos;
	vec3 light_dir = normalize(light_vertex);

	vec3 dat = textureCubeAtlasUV(-light_dir);
	vec2 uv = dat.xy;
	int  face = int(dat.z);

	return SHADOW_FILTER(shadowmap_atlas,
	                     POINT_LIGHT(idx).shadowmap[face].xyz,
	                     light_vertex,
	                     uv);
}

float spot_shadow(uint idx, vec3 pos) {
	vec3 light_vertex = pos - SPOT_LIGHT(idx).position.xyz;
	vec3 light_dir = normalize(light_vertex);
	float light_angle = dot(light_dir, SPOT_LIGHT(idx).direction.xyz);

	// TODO: have start/end angles as part of the spot_light struct
	float k = 0.1;
	float edge = clamp((light_angle - (SPOT_LIGHT(idx).angle))/k, 0.0, 1.0);
	edge *= edge;

	if (!SPOT_LIGHT(idx).casts_shadows) {
		return edge;
	}

	if (light_angle > SPOT_LIGHT(idx).angle) {
		vec4 proj = SPOT_LIGHT(idx).shadowproj * vec4(pos, 1.0);
		vec3 ndc = proj.xyz/proj.w;
		vec2 uv = clamp((ndc.xy)*0.5 + 0.5, 0.0, 1.0);

		vec3 asdf = SPOT_LIGHT(idx).position.xyz - pos;
		float shadow = SHADOW_FILTER(shadowmap_atlas,
		                             SPOT_LIGHT(idx).shadowmap.xyz,
		                             asdf,
		                             uv);

		return edge * shadow;
	}

	return 0.0;
}

float directional_shadow(uint idx, vec3 pos) {
	if (!DIRECTIONAL_LIGHT(idx).casts_shadows) {
		return 1.0;
	}

	// TODO: directional_shadow()
	return 1.0;
}
