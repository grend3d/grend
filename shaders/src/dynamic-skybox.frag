#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>

IN vec3 f_texcoord;
uniform samplerCube skytexture;

//const vec3 lightdir = vec3(0.5774, 0.5774, 0.5774);
//const vec3 lightdir = vec3(0.5345, 0.2673, 0.8018);
const vec3 lightdir = vec3(1, 0, 0);
const vec3 up = vec3(0, 1, 0);
//const vec3 skyblue = vec3(0.25, 0.6, 1.0);
const vec3 skyblue = vec3(0.02, 0.02, 0.02);

vec3 asdf(vec3 foo) {
	return (foo / (foo + vec3(1.0)));
}

void main(void) {
	vec3 coord = normalize(f_texcoord);
	float dirup = max(0.0, dot(up, coord));
	float dirlight = sign(dirup) * max(0.0, dot(lightdir, coord));


	vec3 suncolor = vec3(0.08, 0.08, 0.08);
	vec3 skycolor = skyblue; // TODO: color based on sun position

	vec3 corona = vec3(((1.0 / max(0.1, abs(coord.y)))) * 0.1 * (0.1 + 0.4*dirlight));
	//vec3 light = suncolor * (pow(dirlight, 3.0))*0.5;
	//vec3 sky = (skycolor * (sign(dirup) * (1.0 / max(0.02, coord.y)) * 0.02)) * 8.0;

	vec3 light = suncolor * (pow(dirlight, 3.0))*0.5;
	vec3 sky = (skycolor * dirup);

	//vec4 total = vec4(corona + sky + light, 1.0);
	vec4 total = vec4(sky + light, 1.0);
	vec2 uv2d = vec2(f_texcoord.x + f_texcoord.y, f_texcoord.y + f_texcoord.z);

	FRAG_COLOR = EARLY_TONEMAP(total, 1.0, uv2d);
}
