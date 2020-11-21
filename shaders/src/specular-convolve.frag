#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/atlas_cubemap.glsl>
#include <lib/compat.glsl>
#include <lighting/lambert-diffuse.glsl>

IN vec2 f_texcoord;
uniform vec3 cubeface[6];
uniform int currentFace;
uniform sampler2D reflection_atlas;

void main(void) {
	vec4 sum = vec4(0);
	vec3 curdir = textureCubeAtlasDir(currentFace, f_texcoord);
	float size = float(textureSize(reflection_atlas, 0).x) * cubeface[0].z;
	float inc = 1.0 / size;
	//float len = length(curdir);
	float len = 1.0;
	float factor = rend_x / screen_x;
	float lim = factor * (len / size);
	float div = pow(2.0*factor*len - 1, 2.0);
	float asdf = 4.0;
	
	// TODO: proper GGX distribution sampling
	for (float x = -asdf*lim; x < asdf*lim; x += asdf*inc) {
		for (float y = -asdf*lim; y < asdf*lim; y += asdf*inc) {
			vec3 adjdir = textureCubeAtlasAddUV(currentFace, curdir, vec2(x, y));
			vec4 sample = textureCubeAtlas(reflection_atlas, cubeface, adjdir);
			sum += sample;
		}
	}

	sum -= texture2DAtlas(reflection_atlas, cubeface[currentFace], f_texcoord);
	FRAG_COLOR = vec4(sum.xyz / div, 1.0);
}
