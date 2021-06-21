#define VERTEX_SHADER

#include <lib/compat.glsl>

IN vec3 v_position;
IN vec3 v_color;
OUT vec3 f_color;

uniform mat4 cam;

void main(void) {
	gl_Position = cam * vec4(v_position, 1.0);
	f_color = v_color;
}
