#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>
#include <lib/atlas_cubemap.glsl>

uniform sampler2D irradiance_atlas;
uniform vec3 cubeface[6];

void main(void) {
	FRAG_COLOR = vec4(textureCubeAtlas(irradiance_atlas, cubeface, f_normal).rgb, 1.0);
}
