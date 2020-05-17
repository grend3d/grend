#pragma once
#include <lib/compat.glsl>

#ifdef FRAGMENT_SHADER
in vec3 f_normal;
in vec3 f_tangent;
in vec3 f_bitangent;
in vec2 f_texcoord;
in vec4 f_position;
in mat3 TBN;

#endif

#ifdef VERTEX_SHADER
out vec3 f_normal;
out vec3 f_tangent;
out vec3 f_bitangent;
out vec2 f_texcoord;
out vec4 f_position;
out mat3 TBN;
#endif
