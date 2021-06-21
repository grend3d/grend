#define FRAGMENT_SHADER

#include <lib/compat.glsl>

IN vec3 f_color;

void main(void) {
	// TODO: tweakable opacity uniform
	FRAG_COLOR = vec4(f_color, 0.1);
}
