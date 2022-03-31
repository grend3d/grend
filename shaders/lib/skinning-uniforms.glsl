#pragma once

// no UBOs on gles2
#if GLSL_VERSION < 300
#ifndef MAX_JOINTS
#define MAX_JOINTS 32
#endif

uniform mat4 joints[MAX_JOINTS];

// use UBOs on gles3, core profiles
#else
// TODO: should be configurable, lots of newer GPUs have 64kb UBOs
#define MAX_JOINTS 256

layout (std140) uniform jointTransforms {
	mat4 joints[MAX_JOINTS];
};
#endif /* GLSL_VERSION */

#include <lib/compat.glsl>

// affecting joints and weights
IN vec4 a_joints;
IN vec4 a_weights;
