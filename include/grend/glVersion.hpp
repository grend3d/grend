#pragma once

#if GLSL_VERSION == 100
#define GLSL_STRING "100"
#elif GLSL_VERSION == 300
#define GLSL_STRING "300 es"
#elif GLSL_VERSION == 330
#define GLSL_STRING "330 core"
#elif GLSL_VERSION == 430
#define GLSL_STRING "430 core"
#endif

