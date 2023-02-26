#include <grend/glManager.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/logger.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

// TODO: move
// srgb -> linear, in place
#include <math.h>
static inline void srgb_to_linear(uint8_t *pixels, size_t length) {
	for (size_t i = 0; i < length; i++) {
		pixels[i] = 0xff * pow((pixels[i]/256.0), 2.2);
	}
}

static inline void srgb_to_linear(std::vector<uint8_t>& pixels) {
	for (size_t i = 0; i < pixels.size(); i++) {
		//pixels[i] = 0xff * pow((pixels[i]/256.0), 2.2);
		float a = pixels[i]/255.f;
		pixels[i] = 0xff * a*a; // close enough :P (way faster)
	}
}

void Texture::buffer(textureData::ptr tex) {
	return buffer(*tex.get());
}

static GLenum getInternalTypeFloat(unsigned channels) {
	return (channels == 1)? GL_R16F
	     : (channels == 2)? GL_RG16F
	     : (channels == 3)? GL_RGB16F
	     : (channels == 4)? GL_RGBA16F
	     : GL_RGBA16F;
}

#if GLSL_VERSION != 100 && GLSL_VERSION != 300
static GLenum getInternalTypeUint16(unsigned channels) {
	return (channels == 1)? GL_R16
	     : (channels == 2)? GL_RG16
	     : (channels == 3)? GL_RGB16
	     : (channels == 4)? GL_RGBA16
	     : GL_RGBA16;
}
#endif

static GLenum getInternalTypeUint8(unsigned channels) {
	return (channels == 1)? GL_R8
	     : (channels == 2)? GL_RG8
	     : (channels == 3)? GL_RGB8
	     : (channels == 4)? GL_RGBA8
	     : GL_RGBA8;
}

textureData resample8(const textureData& tex) {
	if (auto *pixels = std::get_if<std::vector<uint8_t>>(&tex.pixels)) {
		return tex;
	}

	else if (auto *pixels = std::get_if<std::vector<uint16_t>>(&tex.pixels)) {
		std::vector<uint8_t> newPixels;
		newPixels.reserve(pixels->size());

		for (uint16_t comp : *pixels) {
			newPixels.push_back(comp >> 8);
		}

		textureData ret;
		ret.pixels = std::move(newPixels);
		return ret;
	}

	else if (auto *pixels = std::get_if<std::vector<float>>(&tex.pixels)) {
		std::vector<uint8_t> newPixels;
		newPixels.reserve(pixels->size());

		for (float comp : *pixels) {
			newPixels.push_back(255 * comp);
		}

		textureData ret;
		ret.pixels = std::move(newPixels);
		return ret;
	}

	return textureData();
}

void Texture::buffer(const textureData& tex) {
	LogFmt(" > buffering image: w = {}, h = {}, channels: {}\n",
	       tex.width, tex.height, tex.channels);

	GLenum texformat = surfaceGlFormat(tex.channels);
	bind();

	if (auto *pixels = std::get_if<std::vector<float>>(&tex.pixels)) {
		// TODO: float isn't valid on gles2 (if I ever get to fixing that)
		GLenum internalType = getInternalTypeFloat(tex.channels);

		LogInfo(" > type: float");
		glTexImage2D(GL_TEXTURE_2D,
		             0, internalType, tex.width, tex.height,
		             0, texformat, GL_FLOAT, pixels->data());
		DO_ERROR_CHECK();
	}

	#if GLSL_VERSION == 100 || GLSL_VERSION == 300
	// gles profiles don't support unsigned short textures, need to resample as unsigned bytes
	else if (std::get_if<std::vector<uint16_t>>(&tex.pixels)) {
		#if GREND_BUILD_DEBUG
			LogWarn(" > WARNING: resampling uint16 texture as uint8, you should avoid "
			        "using 16 bit component textures on OpenGL ES profiles");
		#endif

		textureData newtex = resample8(tex);

		auto *pixels = std::get_if<std::vector<uint8_t>>(&newtex.pixels);
		GLenum internalType = getInternalTypeUint8(tex.channels);

		LogInfo(" > type: unsigned byte (resampled)");
		glTexImage2D(GL_TEXTURE_2D,
		             0, internalType, tex.width, tex.height,
		             0, texformat, GL_UNSIGNED_BYTE, pixels->data());
		DO_ERROR_CHECK();
	}

	#else
	else if (auto *pixels = std::get_if<std::vector<uint16_t>>(&tex.pixels)) {
		GLenum internalType = getInternalTypeUint16(tex.channels);

		LogInfo(" > type: unsigned short");
		glTexImage2D(GL_TEXTURE_2D,
		             0, internalType, tex.width, tex.height,
		             0, texformat, GL_UNSIGNED_SHORT, pixels->data());
		DO_ERROR_CHECK();
	}
	#endif

	else if (auto *pixels = std::get_if<std::vector<uint8_t>>(&tex.pixels)) {
		GLenum internalType = getInternalTypeUint8(tex.channels);

		LogInfo(" > type: unsigned byte");
		glTexImage2D(GL_TEXTURE_2D,
		             0, internalType, tex.width, tex.height,
		             0, texformat, GL_UNSIGNED_BYTE, pixels->data());
		DO_ERROR_CHECK();

	} else {
		LogError("Texture::buffer: Somehow have a pixel format that isn't valid?");
	}

	DO_ERROR_CHECK();

	// initialize with defaults just in case, should never be needed
	GLenum min = GL_LINEAR_MIPMAP_LINEAR;
	GLenum mag = GL_LINEAR;
	GLenum wrapS = GL_REPEAT;
	GLenum wrapT = GL_REPEAT;

	// TODO: tweakable max settings
	switch (tex.minFilter) {
		case textureData::filter::Nearest:
			min = GL_NEAREST;
			break;
		case textureData::filter::Linear:
			min = GL_LINEAR;
			break;

		// TODO: probably want to choose one of these regardless
		//       of what the format specifies, needs to be tweakable
		//       for performance/visuals, blender seems to export with
		//       "nearest_mipmap_linear" which looks terrible
		case textureData::filter::NearestMipmapNearest:
			min = GL_NEAREST_MIPMAP_NEAREST;
			break;
		case textureData::filter::NearestMipmapLinear:
			min = GL_NEAREST_MIPMAP_LINEAR;
			break;
		case textureData::filter::LinearMipmapNearest:
			min = GL_LINEAR_MIPMAP_NEAREST;
			break;
		case textureData::filter::LinearMipmapLinear:
			min = GL_LINEAR_MIPMAP_LINEAR;
			break;

		default: break;
	}

	switch (tex.magFilter) {
		case textureData::filter::Nearest:
			mag = GL_NEAREST;
			break;
		case textureData::filter::Linear:
			mag = GL_LINEAR;
			break;
		default:
			break;
	}

	auto wrapGLFlags = [](textureData::wrap flag) {
		switch (flag) {
			case textureData::wrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
			case textureData::wrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
			case textureData::wrap::Repeat:         return GL_REPEAT;
			default:                                return GL_REPEAT;
		}
	};

	wrapS = wrapGLFlags(tex.wrapS);
	wrapT = wrapGLFlags(tex.wrapT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
#if GLSL_VERSION == 100 || GLSL_VERSION == 300
// TODO: not using glew on embedded profiles, might as well just write
//       my own thing to parse available extensions
#else
	if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
		// anisotropic filtering extension
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
		DO_ERROR_CHECK();
	}
#endif
// defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
#endif

	// if format uses mipmap filtering, generate mipmaps
	if (tex.minFilter >= textureData::filter::NearestMipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	// debug info
	// TODO: use type sizes
	size_t roughsize = tex.width * tex.height * tex.channels * 1.33;
	currentSize = glmanDbgUpdateTextures(currentSize, roughsize);
}

void Texture::cubemap(std::string directory, std::string extension) {
	// TODO: texture cache
	bind(GL_TEXTURE_CUBE_MAP);
	//tex->bind(GL_TEXTURE_CUBE_MAP);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, tex.first);
	DO_ERROR_CHECK();

	std::pair<std::string, GLenum> dirmap[] =  {
		{"negx", GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
		{"negy", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
		{"negz", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
		{"posx", GL_TEXTURE_CUBE_MAP_POSITIVE_X},
		{"posy", GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
		{"posz", GL_TEXTURE_CUBE_MAP_POSITIVE_Z}
	};

	LogFmt("Loading a cubemap from {}", directory);

	for (const auto& [img, direction] : dirmap) {
		std::string fname = directory + "/" + img + extension;
		textureData tex;

		if (!tex.load_texture(fname)) {
			LogErrorFmt("Couldn't load cubemap text from '{}'", fname);
			continue;
		}

		LogFmt(" > loaded image: w = {}, h = {}, pitch = {}, bytesperpixel: {}\n",
		       tex.width, tex.height, 0, tex.channels);

		GLenum texformat = surfaceGlFormat(tex.channels);

		if (auto *pixels = std::get_if<std::vector<float>>(&tex.pixels)) {
			glTexImage2D(direction, 0, GL_RGBA, tex.width, tex.height, 0,
			             texformat, GL_FLOAT, pixels->data());
		}

		else if (auto *pixels = std::get_if<std::vector<uint16_t>>(&tex.pixels)) {
			glTexImage2D(direction, 0, GL_SRGB, tex.width, tex.height, 0,
			             texformat, GL_UNSIGNED_SHORT, pixels->data());
		}

		else if (auto *pixels = std::get_if<std::vector<uint8_t>>(&tex.pixels)) {
			glTexImage2D(direction, 0, GL_SRGB, tex.width, tex.height, 0,
			             texformat, GL_UNSIGNED_BYTE, pixels->data());

		} else {
			LogErrorFmt("{}: Somehow have a pixel format that isn't valid?", __func__);
		}

		DO_ERROR_CHECK();
		LogFmt("Loaded {} to cubemap", fname);
	}

#if 1
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
#endif

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	DO_ERROR_CHECK();

	//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#if GLSL_VERSION >= 130
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
#endif
	DO_ERROR_CHECK();
}

// namespace grendx
}
