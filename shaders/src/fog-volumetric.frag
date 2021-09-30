#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>

//IN vec2 f_texcoord;

vec4 fogColor = vec4(vec3(0.4), 1.0);

uniform vec3 cameraPos;
uniform vec3 cameraForward;
uniform vec3 cameraUp;
uniform vec3 cameraRight;

#include <lib/shadows.glsl>
#include <lib/attenuation.glsl>
#include <lib/tonemapping.glsl>

// TODO: camera uniforms
const float lnear = 0.1;
const float lfar = 1000.0;

float linearDepth(float d) {
	return (2.0*lfar*lnear)/(lfar+lnear - (d*2.0 - 1.0)*(lfar - lnear));
}

#define ITERS 8

#include <lib/noise.glsl>

float hash13(vec3 p3) {
	p3  = fract(p3 * .1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

vec3 raymarch(vec3 pos, vec3 dir, float depth, uint cluster) {
	//float n = uniformNoise(f_texcoord*vec2(screen_x, screen_y), time_ms).x;
	float n = hash13(vec3(f_texcoord*vec2(screen_x, screen_y), time_ms/1000.0));
	float inc = 1.0 / float(ITERS);
	float t = inc * n * depth;
	vec3 accum = vec3(0);

	for (uint i = uint(0); i < uint(ITERS); i++) {
		vec3 p = pos + dir*t;

		for (uint m = uint(0); m < ACTIVE_POINTS(cluster); m++) {
			float atten = point_attenuation(m, p);
			float shadow = point_shadow(m, p);

			accum += inc * 0.25*atten*shadow * POINT_LIGHT(m).diffuse.rgb;
		}

		for (uint m = uint(0); m < ACTIVE_SPOTS(cluster); m++) {
			float atten = spot_attenuation(m, p);
			float shadow = spot_shadow(m, p);

			accum += inc * 0.25*atten*shadow * SPOT_LIGHT(m).diffuse.rgb;
		}


		t += inc*depth;
	}

	return accum;
}

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;
	float aspectRatio = screen_x / screen_y;

	vec4 color = texture2D(render_fb, scaled_texcoord);
	float depth = texture2D(render_depth, scaled_texcoord).r;

	vec2 duv = f_texcoord*2.0 - 1.0;
	//vec2 duv = f_texcoord;
	vec3 uuv = cameraPos
		+ cameraForward*1.0
		+ f_texcoord.x * cameraRight
		+ f_texcoord.y * cameraUp
		;
	//vec3 dir = normalize(uuv - cameraPos);

	//float asdf = tan(1.2217 * 0.5);
	//float asdf = tan(70.0 * 0.5 * 3.141592/180.0);
	float asdf = 0.7;

	vec3 dir = normalize(
		cameraForward/asdf
		+ duv.x * -cameraRight
		+ duv.y/aspectRatio * cameraUp
	);

	uint cluster = SCREEN_TO_CLUSTER(f_texcoord.x, 1.0 - f_texcoord.y);
	//uint cluster = SCREEN_TO_CLUSTER(gl_FragCoord.x / screen_x, (screen_y - gl_FragCoord.y) / screen_y);

	//float d = (depth < 1.0)? 50.0 : linearDepth(depth);
	float d = linearDepth(depth);

	vec3 fog = 0.05*raymarch(cameraPos, dir, d, cluster);
	vec3 undone = undoTonemap(color, exposure).rgb;
	fog = reinhard_hdr_modified(undone + fog, exposure);
	FRAG_COLOR = vec4(fog.rgb, 1.0);
}
