#pragma once
#include <grend-config.h>

#if GLSL_VERSION == 100 || GLSL_VERSION == 300
#define NO_FORMAT_CONVERSION
#define NO_FLOATING_FB
#endif

#if defined(_WIN32) || defined(_WIN64)
// always use glew on windows
#include <windef.h>
#include <GL/glew.h>

#else /* android, unixen */

// use glew for core versions, GLES headers for ES
#if GLSL_VERSION == 100
#define GL2_PROTOTYPES 1
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#elif GLSL_VERSION == 300
#define GL3_PROTOTYPES 1
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#else
#include <GL/glew.h>
#endif

#endif /* android, unixen */
