#pragma once
#include <lib/compat.glsl>

#ifdef FRAGMENT_SHADER
IN vec3 f_normal;
IN vec3 f_tangent;
IN vec3 f_bitangent;
IN vec2 f_texcoord;
IN vec4 f_position;
IN mat3 TBN;
#endif

#ifdef VERTEX_SHADER
// vertex attributes
IN vec3 in_Position;
IN vec2 texcoord;
IN vec3 v_normal;
IN vec3 v_tangent;
IN vec3 v_bitangent;

OUT vec3 f_normal;
OUT vec3 f_tangent;
OUT vec3 f_bitangent;
OUT vec2 f_texcoord;
OUT vec4 f_position;
OUT mat3 TBN;
#endif
