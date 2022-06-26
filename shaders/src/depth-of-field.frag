#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>
#include <lib/camera-uniforms.glsl>
#include <lib/davehash.glsl>

IN vec2 f_texcoord;

uniform vec4 levels;
 
vec2 vogelDisk(float scale, float index) {
    float r = scale*sqrt(index);
    float theta = index * 2.3998; // radians(137.5)
    return vec2(
        r*cos(theta),
        r*sin(theta)
    );
}

float linearDepth(float d) {
	//return (2.0*far*near)/(far+near - (d*2.0 - 1.0)*(far - near));
	return (2.0*1000.0*0.1)/(1000.0+0.1 - (d*2.0 - 1.0)*(1000.0 - 0.1));
}

//#define SAMPLES 64
//#define SAMPLES 32
#define SAMPLES 16
#define DISK_SCALE 0.04

//void main( out vec4 fragColor, in vec2 fragCoord )
void main(void) {
    vec4 sum = vec4(0);
    float samples = 0.0;

	vec2 uv = f_texcoord;
    float asp = screen_x / screen_y;

    // 2.61... constant comes from 2pi / radians(137.5), where 137.5 degrees
    // is the angle increment for each index in the vogel disk,
    // this way the disk will be rotated between 0 and 2pi radians
	// TODO: blue noise texture
    float k = 2.616885*hash12(12345.0*uv);

	float depth = linearDepth(texture(render_depth, uv).r);
	vec4 scol = texture(render_fb, uv);
	sum += scol;
	samples += 1;

	// maximum extent of vogel disk for this number of samples
	float extent = sqrt(float(SAMPLES - 1));

    for (int i = 0; i < SAMPLES; i++) {
        vec2 vog = vogelDisk(1.0, float(i)+k);
        vog.x /= asp;

		float focus = 3.0;
		float dist = depth - focus;
		float lens = 4.4;
		float disk = 1.0 - lens/sqrt(lens*lens + dist*dist);
        vec2 ruv = uv + DISK_SCALE*vog/extent;

        // avoid sampling beyond edges of the screen
        if (min(ruv.x, ruv.y) >= 0.0 && max(ruv.x, ruv.y) <= 1.0) {
			float sdepth = linearDepth(texture(render_depth, ruv).r);
			float sdist = sdepth - focus;
			float sdisk = 1.0 - lens/sqrt(lens*lens + sdist*sdist);

			if (depth >= (sdepth - 2.0) && length((uv - ruv) / vec2(1.0, asp)) < DISK_SCALE*sdisk) {
				vec4 col = texture(render_fb, ruv);

				sum += col;
				samples += 1.0;
			}
        }
    }

    vec4 temp = sum / samples;
    // Output to screen
    FRAG_COLOR = vec4(temp.rgb, 1.0);
}
