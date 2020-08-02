#pragma once
#include <lib/shading-uniforms.glsl>
#include <lib/atlas_cubemap.glsl>

// TODO: make these uniforms
float near = 0.1;
float far = 100.0;

// TODO: compile-time and uniform parameter macros, once I get to
//       doing hot shader reloading
#define DEPTH_BIAS 0.0005
#define ENABLE_SHADOW_FILTERING 1
#define SHADOW_FILTER shadow_pcf4

// https://stackoverflow.com/questions/10786951/omnidirectional-shadow-mapping-with-depth-cubemap
float vec_to_depth(vec3 vec) {
	vec3 ab = abs(vec);
	float z_comp = max(ab.x, max(ab.y, ab.z));
	float norm_z_comp = (far+near) / (far-near) - (2.0*far*near)/(far-near)/z_comp;

	return (norm_z_comp + 1.0) * 0.5;
}

// 4 sample PCF (pretty fast, this would be the "very low" setting)
float shadow_pcf4(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float ret = 0.0;
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float off_factor = 1.0/texsize;

	for (int x = 0; x <= 1; x++) {
		for (int y = 0; y <= 1; y++) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
		}
	}

	return ret/4.0;
}

// 4 sample PCF (x2, interpolated)
float shadow_pcf4_interp(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float ret = 0.0;
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float off_factor = 1.0/texsize;

	for (int x = 0; x <= 1; x++) {
		for (int y = 0; y <= 1; y++) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
		}
	}

	for (float x = -1.5; x <= 1.5; x += 3.0) {
		for (float y = -1.5; y <= 1.5; y += 3.0) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 2.0 : 0.0;
		}
	}

	return ret/12.0;
}

// testing pcf4 with float offsets to see if linear interpolation does anything
float shadow_pcf4_alt(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float ret = 0.0;
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float off_factor = 1.0/texsize;

	for (float x = -0.5; x <= 0.5; x++) {
		for (float y = -0.5; y <= 0.5; y++) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
		}
	}

	return ret/4.0;
}

// 9 sample PCF (x2, interpolated)
float shadow_pcf9_interp(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float ret = 0.0;
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float off_factor = 1.0/texsize;

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
		}
	}

	for (float x = -1.5; x <= 1.5; x += 1.5) {
		for (float y = -1.5; y <= 1.5; y += 1.5) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 2.0 : 0.0;
		}
	}

	return ret/27.0;
}

// 9 sample PCF
float shadow_pcf9(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float ret = 0.0;
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float off_factor = 1.0/texsize;

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
		}
	}

	return ret/9.0;
}

// 16 sample PCF
float shadow_pcf16(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float ret = 0.0;
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float off_factor = 1.0/texsize;

	for (float x = -1.5; x <= 1.5; x++) {
		for (float y = -1.5; y <= 1.5; y++) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
		}
	}

	return ret/16.0;
}

// 32 sample PCF
float shadow_pcf32(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float ret = 0.0;
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float off_factor = 1.0/texsize;

	for (float x = -1.75; x <= 1.75; x += 0.5) {
		for (float y = -1.5; y <= 1.5; y++) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
		}
	}

	return ret/32.0;
}

// 64 sample PCF
float shadow_pcf64(in sampler2D atlas, vec3 slice, vec3 vert, vec2 uv) {
	float ret = 0.0;
	float texsize = float(textureSize(atlas, 0).x) * slice.z;
	float off_factor = 1.0/texsize;

	for (float x = -1.75; x <= 1.75; x += 0.5) {
		for (float y = -1.75; y <= 1.75; y += 0.5) {
			vec2 off = uv + vec2(x, y)*off_factor;
			float depth = texture2DAtlas(atlas, slice, off).r;

			ret += ((depth + DEPTH_BIAS) > vec_to_depth(vert))? 1.0 : 0.0;
		}
	}

	return ret/64.0;
}

float point_shadow(int i, vec3 pos) {
	// TODO: can branching be avoided without the texture sample?
	if (!point_lights[i].casts_shadows) {
		return 1.0;
	}

	vec3 light_vertex = point_lights[i].position - pos;
	vec3 light_dir = normalize(light_vertex);

#if ENABLE_SHADOW_FILTERING
	vec3 cubedat = textureCubeAtlasUV(-light_dir);
	int face = int(cubedat.z);
	vec2 uv = vec2(cubedat);

	return SHADOW_FILTER(shadowmap_atlas, point_lights[i].shadowmap[face],
	                     light_vertex, uv);

#else
	vec4 depth = textureCubeAtlas(shadowmap_atlas,
								  point_lights[i].shadowmap, -light_dir);

	return ((depth.r + DEPTH_BIAS) > vec_to_depth(light_vertex))? 1.0 : 0.0;
#endif
}

float spot_shadow(int i, vec3 pos) {
	if (!spot_lights[i].casts_shadows) {
		return 1.0;
	}

	vec3 light_vertex = spot_lights[i].position - pos;
	vec3 light_dir = normalize(light_vertex);

	if (dot(light_dir, spot_lights[i].direction) > spot_lights[i].angle) {
		// XXX: maybe pass this in a uniform, or use quarternion for rotation
		//      and extract an SO(3) out of that
		vec3 adjdir = normalize(-spot_lights[i].direction);
		vec3 right = normalize(cross(vec3(0, 0, 1), adjdir));
		vec3 up = normalize(cross(right, adjdir));

		float p = 1.0 - spot_lights[i].angle;
		//vec2 uv = (vec2(light_dir.x, -light_dir.z) + 1.0) / 2.0;
		vec2 uv = (vec2(dot(light_dir, right), dot(light_dir, up)) + 1.0) / 2.0;
		//vec2 uv = (vec2(dot(light_dir, right), dot(light_dir, up)) + 1.0) / 2.0;
		vec4 depth = texture2DAtlas(shadowmap_atlas,
		                            spot_lights[i].shadowmap, uv);

		return ((depth.r + 0.001) > vec_to_depth(light_vertex))? 1.0 : 0.0;
	}

	return 0.0;
}

float directional_shadow(int i, vec3 pos) {
	if (!directional_lights[i].casts_shadows) {
		return 1.0;
	}

	// TODO: directional_shadow()
	return 1.0;
}
