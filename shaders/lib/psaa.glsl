// pseudo antialiasing
// TODO: actually add SMAA/FXAA :P
#include <lib/postprocessing-uniforms.glsl>
#define CONTRAST_SCALE 2.0

float greyscale_sample(in sampler2D buf, vec2 uv) {
        vec3 color = texture2D(buf, uv).xyz;
        return 0.3*color.r + 0.5*color.g + 0.2*color.b;
}

// Just for quickly testing things here...
#ifndef PSAA_SAMPLES
#define PSAA_SAMPLES 2
#endif

float ps_contrast(in sampler2D buf, vec2 uv) {
        float sum = 0.0;
        float pixel_x = 2.33 / screen_x;
        float pixel_y = 2.33 / screen_y;

		// sample horizontally/vertically for contrast
#if PSAA_SAMPLES == 4
        sum += greyscale_sample(buf, uv + vec2(-pixel_x, 0.0));
        sum += greyscale_sample(buf, uv + vec2(0.0, -pixel_y));
#endif
        sum += greyscale_sample(buf, uv + vec2(0.0, pixel_y));
        sum += greyscale_sample(buf, uv + vec2(pixel_x, 0.0));

        //return max(0.0, sum/8.0 - greyscale_sample(buf, uv));
#if PSAA_SAMPLES == 4
        return abs(greyscale_sample(buf, uv) - sum/4.0);
#else
        return abs(greyscale_sample(buf, uv) - sum/2.0);
#endif
}

vec4 ps_blur(in sampler2D buf, vec2 uv, float strength) {
        vec4 sum = vec4(0.0);
        float pixel_x = 1.5 / screen_x;
        float pixel_y = 1.5 / screen_y;

        vec4 center = texture2D(buf, uv);

		// sample diagonally for blur
#if PSAA_SAMPLES == 4
        sum += texture2D(buf, uv + vec2(-pixel_x, -pixel_y));
        sum += texture2D(buf, uv + vec2(-pixel_x, pixel_y));
#endif
        sum += texture2D(buf, uv + vec2(pixel_x, -pixel_y));
        sum += texture2D(buf, uv + vec2(pixel_x, pixel_y));

#if PSAA_SAMPLES == 4
        return mix(center, sum / 4.0, strength);
#else
        return mix(center, sum / 2.0, strength);
#endif
}

vec4 psaa(vec2 texcoord) {
	float contrast = CONTRAST_SCALE*ps_contrast(render_fb, texcoord);

#ifdef DEBUG_PSAA
	vec4 color = texture2D(render_fb, texcoord);
	return vec4(color.rgb + vec3(0, contrast, 0), 1.0);

#else
	return ps_blur(render_fb, texcoord, contrast);
#endif
}
