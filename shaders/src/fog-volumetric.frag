#define FRAGMENT_SHADER

uniform sampler2D blueNoise;

#include <lib/compat.glsl>
#include <lib/constants.glsl>
#include <lib/postprocessing-uniforms.glsl>
#include <lib/camera-uniforms.glsl>

#include <lib/shadows.glsl>
#include <lib/attenuation.glsl>
#include <lib/tonemapping.glsl>

IN vec2 f_texcoord;

// TODO: camera uniforms
const float lnear = 0.1;
const float lfar = 1000.0;

float linearDepth(float d) {
	return (2.0*lfar*lnear)/(lfar+lnear - (d*2.0 - 1.0)*(lfar - lnear));
}

// TODO: Figure out how to expose this as a quality setting
//#define ITERS 1
#define ITERS 2
//#define ITERS 4
//#define ITERS 8

uniform mat4 p_inv;

// TODO:
//vec4 fogColor = vec4(vec3(0.4), 1.0);

uniform float fogStrength;
uniform float fogAbsorption;
uniform float fogConcentration;
uniform float fogAmbient;

float lengthsq(vec3 v) {
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

vec3 raymarch(vec3 pos, vec3 dir, float depth, uint cluster) {
	float n = texture2D(blueNoise, (fract(GOLDEN_RATIO*60.0*time_ms/1000.0)) + gl_FragCoord.xy/textureSize(blueNoise, 0)).x;
	float inc = 1.0 / float(ITERS);
	float stepLen = inc*depth;
	float s = n*stepLen;

	vec3 accum = vec3(depth * fogAmbient * (fogAbsorption*fogConcentration));

	for (uint i = uint(0); i < uint(ITERS); i++) {
		float k = float(i)*(stepLen);
		vec3 off = dir*s + dir*k;
		vec3 p = pos + off;

		if (lengthsq(off) > depth*depth) break;

		for (uint m = uint(0); m < ACTIVE_POINTS(cluster); m++) {
			uint idx = POINT_LIGHT_IDX(m, cluster);

			float foo = fogAbsorption*fogConcentration*length(p - POINT_LIGHT(idx).position.xyz);

			float atten = point_attenuation(idx, p) * pow(max(0.0, 1.0 - foo), 2.0);
			float shadow = point_shadow(idx, p);

			accum += inc * atten*shadow * POINT_LIGHT(idx).diffuse.rgb;
		}

		for (uint m = uint(0); m < ACTIVE_SPOTS(cluster); m++) {
			uint idx = SPOT_LIGHT_IDX(m, cluster);

			float foo = fogAbsorption*fogConcentration*length(p - SPOT_LIGHT(idx).position.xyz);

			float atten = spot_attenuation(idx, p) * pow(max(0.0, 1.0 - foo), 2.0);
			float shadow = spot_shadow(idx, p);

			accum += inc * atten*shadow * SPOT_LIGHT(idx).diffuse.rgb;
		}
	}

	return accum;
}

void main(void) {
	vec2 scaled_texcoord = vec2(scale_x, scale_y) * f_texcoord;
	vec2 viewport_uv = (vec2(gl_FragCoord.x, screen_y - gl_FragCoord.y) - drawOffset)/drawSize;
	viewport_uv.y = 1.0 - viewport_uv.y;

	vec4 color = texture2D(render_fb, scaled_texcoord);
	float depthSample = texture2D(render_depth, scaled_texcoord).r;
	float depth = linearDepth(depthSample);

	vec2 duv = viewport_uv*2.0 - 1.0;
	float aspectX = drawSize.x / drawSize.y;
	float perspOffY = tan(cameraFovy * PI/360.0);

	vec2 uv = viewport_uv;
	vec3 dir = mat3(v_inv)*vec3(duv.x*aspectX*perspOffY, duv.y * perspOffY, -1.0);
	uint cluster = SCREEN_TO_CLUSTER(gl_FragCoord.x/screen_x, gl_FragCoord.y/screen_y);

	vec3 fog = fogStrength*raymarch(cameraPosition, dir, depth, cluster);

	FRAG_COLOR = vec4(color.rgb + fog.rgb, 1.0);
}
