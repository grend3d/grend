#pragma once

// use glew for core versions, GLES headers for ES
#if GLSL_VERSION == 100
#define GL2_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define NO_FORMAT_CONVERSION

#elif GLSL_VERSION == 300
#define GL3_PROTOTYPES 1
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#define NO_FORMAT_CONVERSION

#else
#include <GL/glew.h>
#endif
