#pragma once
#ifndef MAX_JOINTS
#define MAX_JOINTS 32
#warning "Maximum number of joints should be defined in SHADER_FLAGS"
#endif

#include <lib/compat.glsl>
uniform mat4 joints[MAX_JOINTS];
// affecting joints and weights
IN vec4 a_joints;
IN vec4 a_weights;
