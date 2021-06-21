#define FRAGMENT_SHADER

#include <lib/compat.glsl>

IN vec3 f_color;

void main(void) {
	FRAG_COLOR = vec4(f_color, 1.0);
}
