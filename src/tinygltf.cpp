#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_EXTERNAL_IMAGE

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <windef.h>
#endif

// XXX: ?
#undef _WIN32

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <tinygltf/tiny_gltf.h>
