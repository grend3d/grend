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
	float div = size*size*4;
	float inc = 1.0 / size;

	for (int i = 0; i < 6; i++) {
		for (float x = 0.0; x < 1.0; x += inc) {
			for (float y = 0.0; y < 1.0; y += inc) {
				vec2 uv = vec2(x, y);
				vec3 dir = textureCubeAtlasDir(i, uv);
				vec4 sample = texture2DAtlas(reflection_atlas, cubeface[i], uv);
				float diffuse = lambert_diffuse(0.0, dir, curdir, -dir);

				sum += sample*diffuse;
			}
		}
	}

	FRAG_COLOR = vec4(sum.xyz / div / length(curdir), 1.0);
	//FRAG_COLOR = vec4(f_texcoord, 0, 1.0);
	//FRAG_COLOR = vec4(vec3(length(curdir)), 1.0);
	//FRAG_COLOR = vec4(vec3(float(currentFace)/6.0), 1.0);
}
