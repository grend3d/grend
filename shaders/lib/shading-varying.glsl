#pragma once
#include <lib/compat.glsl>

#ifdef FRAGMENT_SHADER
IN vec4 f_position;
IN vec3 f_normal;
IN vec4 f_tangent;
IN vec4 f_bitangent;
IN vec4 f_color;

IN vec2 f_texcoord;
IN vec2 f_lightmap;
IN mat3 TBN;
#endif

#ifdef VERTEX_SHADER
// vertex attributes
IN vec3 in_Position;
IN vec3 v_normal;
IN vec4 v_tangent;
IN vec4 v_color;

IN vec2 texcoord;
IN vec2 a_lightmap;

OUT vec4 f_position;
OUT vec3 f_normal;
OUT vec4 f_tangent;
OUT vec4 f_bitangent;
OUT vec4 f_color;
OUT vec2 f_texcoord;
OUT vec2 f_lightmap;
OUT mat3 TBN;
#endif
