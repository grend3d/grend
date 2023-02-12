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

void Texture::buffer(textureData::ptr tex, bool srgb) {
	LogFmt(" > buffering image: w = {}, h = {}, bytesperpixel: {}\n",
	       tex->width, tex->height, tex->channels);

	GLenum texformat = surfaceGlFormat(tex->channels);
	bind();

	type = tex->type;
	if (type == textureData::imageType::VecTex) {
		// vectex relies on exact pixel colors, avoid srgb conversion
		srgb = false;
	}

#ifdef NO_FORMAT_CONVERSION
	// XXX: need something more efficient
	std::vector<uint8_t> temp = tex->pixels;

	if (srgb) {
		srgb_to_linear(temp);
	}

	// TODO: fallback SRBG conversion
	glTexImage2D(GL_TEXTURE_2D,
	             //0, srgb? GL_SRGB_ALPHA : GL_RGBA, tex.width, tex.height,
	             0, texformat, tex->width, tex->height,
	             0, texformat, GL_UNSIGNED_BYTE, temp.data());

	/*
	glTexImage2D(GL_TEXTURE_2D,
	             0, texformat, tex->width, tex->height,
	             0, texformat, GL_UNSIGNED_BYTE, tex->pixels.data());
				 */

#else
	glTexImage2D(GL_TEXTURE_2D,
	             0, srgb? GL_SRGB_ALPHA : GL_RGBA, tex->width, tex->height,
	             0, texformat, GL_UNSIGNED_BYTE, tex->pixels.data());
#endif

	DO_ERROR_CHECK();

	// initialize with defaults just in case, should never be needed
	GLenum min = GL_LINEAR_MIPMAP_LINEAR;
	GLenum mag = GL_LINEAR;
	GLenum wrapS = GL_REPEAT;
	GLenum wrapT = GL_REPEAT;

	// TODO: tweakable max settings
	switch (tex->minFilter) {
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

	switch (tex->magFilter) {
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

	wrapS = wrapGLFlags(tex->wrapS);
	wrapT = wrapGLFlags(tex->wrapT);

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
	if (tex->minFilter >= textureData::filter::NearestMipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	// debug info
	size_t roughsize = tex->pixels.size() * 1.33;
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
		//SDL_Surface *surf = IMG_Load(fname.c_str());
		int width, height, channels;
		uint8_t *surf = stbi_load(fname.c_str(), &width, &height, &channels, 0);
		LogFmt("Have cubemap image at {}", fname);

		if (!surf) {
			SDL_Die("couldn't load cubemap texture");
		}

		LogFmt(" > loaded image: w = {}, h = {}, pitch = {}, bytesperpixel: {}\n",
		       width, height, 0, channels);

		GLenum texformat = surfaceGlFormat(channels);

#ifdef NO_FORMAT_CONVERSION
		srgb_to_linear(surf, width*height*channels);
		glTexImage2D(direction, 0, texformat, width, height, 0,
			texformat, GL_UNSIGNED_BYTE, surf);
#else
		glTexImage2D(direction, 0, GL_SRGB, width, height, 0,
			texformat, GL_UNSIGNED_BYTE, surf);
#endif
		DO_ERROR_CHECK();

		//SDL_FreeSurface(surf);
		stbi_image_free(surf);
		LogFmt("Loaded {} to cubemap", fname);

#if 0
		for (unsigned i = 1; surf; i++) {
			std::string mipname = directory + "/mip" + std::to_string(i)
			                      + "-" + img + extension;
			std::cerr << " > looking for mipmap " << mipname << std::endl;

			surf = stbi_load(mipname.c_str(), &width, &height, &channels, 0);
			if (!surf) break;

			// maybe this?
			/*
			if (!surf){
				glTexParameteri(direction, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(direction, GL_TEXTURE_MAX_LEVEL, i - 1);
				DO_ERROR_CHECK();
				break;
			}
			*/

			fprintf(stderr, " > loaded image: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
				width, height, 0, channels);


			GLenum texformat = surfaceGlFormat(channels);
#ifdef NO_FORMAT_CONVERSION
			// again, SRGB fallback, also maybe have a function to init textures
			glTexImage2D(direction, i, texformat, width, height, 0,
				texformat, GL_UNSIGNED_BYTE, surf);
#else
			glTexImage2D(direction, i, GL_SRGB, width, height, 0,
				texformat, GL_UNSIGNED_BYTE, surf);
#endif

			DO_ERROR_CHECK();

			stbi_image_free(surf);
		}
#endif
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
