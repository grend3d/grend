#define FRAGMENT_SHADER

// new version heavily influenced by the FXAA implementation from here:
// http://www.theorangeduck.com/page/unsharp-anti-aliasing
//
// main difference is that I'm using luminance rather than pure pixel difference,
// similar to nvidia's FXAA.

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>

IN vec2 f_texcoord;

//#define DEBUG_PSAA
#define CONTRAST_SCALE 3.0
#define SHARPEN_SCALE 0.75
#define LOW_CUTOFF  0.2
#define HIGH_CUTOFF 0.8
#define CONTRAST_PX 1.0
#define BLUR_PX     1.0

float greydiff(vec4 foo, vec4 bar) {
	float r = 0.3*foo.r - 0.3*bar.r;
	float g = 0.5*foo.g - 0.5*bar.g;
	float b = 0.2*foo.b - 0.2*bar.b;

	return abs(dot(vec3(1), vec3(r, g, b)));
}

// Just for quickly testing things here...
#ifndef PSAA_SAMPLES
#define PSAA_SAMPLES 9
#endif

vec4 ps_blur(in sampler2D buf, vec2 uv) {
	float pixel_x = BLUR_PX / screen_x;
	float pixel_y = BLUR_PX / screen_y;
	vec4 center = texture2D(buf, uv);

#if PSAA_SAMPLES == 9
	vec4 average = vec4(0);

	average += texture2D(buf, uv + vec2(-pixel_x, -pixel_y));
	average += texture2D(buf, uv + vec2(-pixel_x, 0));
	average += texture2D(buf, uv + vec2(-pixel_x, pixel_y));

	average += texture2D(buf, uv + vec2(0, -pixel_y));
	average += texture2D(buf, uv + vec2(0, pixel_y));

	average += texture2D(buf, uv + vec2(pixel_x, -pixel_y));
	average += texture2D(buf, uv + vec2(pixel_x, 0));
	average += texture2D(buf, uv + vec2(pixel_x, pixel_y));
	average /= 8.0;

	float diff = greydiff(center, average);
	float strength = CONTRAST_SCALE*greydiff(center, average);
	center += (center - average)*SHARPEN_SCALE;

	if (strength > LOW_CUTOFF) {
		return mix(center, average, clamp(strength, LOW_CUTOFF, HIGH_CUTOFF));
	} else {
		return center;
	}

// TODO: redo these
#elif PSAA_SAMPLES == 4
	vec4 sum = vec4(0.0);
	sum += texture2D(buf, uv + vec2(-pixel_x, -pixel_y));
	sum += texture2D(buf, uv + vec2(-pixel_x, pixel_y));
	sum += texture2D(buf, uv + vec2(pixel_x, -pixel_y));
	sum += texture2D(buf, uv + vec2(pixel_x, pixel_y));
	return mix(center, sum / float(PSAA_SAMPLES), strength);

#else
	vec4 sum = vec4(0.0);
	sum += texture2D(buf, uv + vec2(pixel_x, -pixel_y));
	sum += texture2D(buf, uv + vec2(pixel_x, pixel_y));
	return mix(center, sum / float(PSAA_SAMPLES), strength);
#endif
}

// implementation from theorangeduck
vec4 altps(in sampler2D buf, vec2 uv) {
	float pixel_x = BLUR_PX / screen_x;
	float pixel_y = BLUR_PX / screen_y;
	vec4 sum = vec4(0.0);
	vec4 center = texture2D(buf, uv + vec2(0, 0));

	sum += texture2D(buf, uv + vec2(-pixel_x, -pixel_y));
	sum += texture2D(buf, uv + vec2(-pixel_x, 0));
	sum += texture2D(buf, uv + vec2(-pixel_x, pixel_y));

	sum += texture2D(buf, uv + vec2(0, -pixel_y));
	sum += texture2D(buf, uv + vec2(0, pixel_y));

	sum += texture2D(buf, uv + vec2(pixel_x, -pixel_y));
	sum += texture2D(buf, uv + vec2(pixel_x, 0));
	sum += texture2D(buf, uv + vec2(pixel_x, pixel_y));

	vec4 average = sum / 8.0;
	vec4 vdiff = center - average;
	float diff = abs(dot(vec4(1), vdiff));

	center += vdiff*0.5;

	if (diff > 0.2) {
		return mix(center, average, clamp(diff, 0.25, 0.75));
	} else {
		return center;
	}
}

vec4 psaa(vec2 texcoord) {
	return ps_blur(render_fb, texcoord);
	//return altps(render_fb, texcoord);
}

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;
	//vec4 color = texture2D(render_fb, scaled_texcoord);
	//FRAG_COLOR = vec4(color.rgb, 1.0);
	FRAG_COLOR = vec4(vec3(psaa(scaled_texcoord)), 1.0);
}
