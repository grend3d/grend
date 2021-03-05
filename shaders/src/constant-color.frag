// similar to unshaded, except this doesn't use material info,
// color is set in the 'outputColor' uniform
#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/shading-varying.glsl>

uniform vec4 lols;
uniform vec4 outputColor;

void main(void) {
	FRAG_COLOR = outputColor;
}
