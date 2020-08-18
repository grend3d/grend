#include <grend/gl_manager.hpp>
#include <grend/glm-includes.hpp>

#include <tinygltf/stb_image.h>
#include <tinygltf/stb_image_write.h>

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>

namespace grendx {

void Texture::buffer(const materialTexture& tex, bool srgb) {
	fprintf(stderr, " > buffering image: w = %u, h = %u, bytesperpixel: %u\n",
	        tex.width, tex.height, tex.channels);

	GLenum texformat = surface_gl_format(tex);
	bind();

#ifdef NO_FORMAT_CONVERSION
	// TODO: fallback SRBG conversion
	glTexImage2D(GL_TEXTURE_2D,
	             //0, srgb? GL_SRGB_ALPHA : GL_RGBA, tex.width, tex.height,
	             0, texformat, tex.width, tex.height,
	             0, texformat, GL_UNSIGNED_BYTE, tex.pixels.data());

#else
	glTexImage2D(GL_TEXTURE_2D,
	             0, srgb? GL_SRGB_ALPHA : GL_RGBA, tex.width, tex.height,
	             0, texformat, GL_UNSIGNED_BYTE, tex.pixels.data());
#endif

	DO_ERROR_CHECK();

#if ENABLE_MIPMAPS
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	DO_ERROR_CHECK();
	glGenerateMipmap(GL_TEXTURE_2D);
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
	DO_ERROR_CHECK();

	/*
	SDL_FreeSurface(texture);

	texture_cache[filename] = temp;
	*/
	// TODO:
	//texture_cache[texhash] = temp;
	//return temp;
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

	std::cerr << __func__ << ": hi" << std::endl;

	for (const auto& [img, direction] : dirmap) {
		std::string fname = directory + "/" + img + extension;
		//SDL_Surface *surf = IMG_Load(fname.c_str());
		int width, height, channels;
		uint8_t *surf = stbi_load(fname.c_str(), &width, &height, &channels, 0);
		std::cerr << __func__ << ": loaded a " << fname << std::endl;

		if (!surf) {
			SDL_Die("couldn't load cubemap texture");
		}

		fprintf(stderr, " > loaded image: w = %u, h = %u, pitch = %u, bytesperpixel: %u\n",
	        width, height, 0, channels);

		GLenum texformat = surface_gl_format(channels);

#ifdef NO_FORMAT_CONVERSION
		// TODO: fallback srgb conversion
		glTexImage2D(direction, 0, texformat, width, height, 0,
			texformat, GL_UNSIGNED_BYTE, surf);
#else
		glTexImage2D(direction, 0, GL_SRGB, width, height, 0,
			texformat, GL_UNSIGNED_BYTE, surf);
#endif
		DO_ERROR_CHECK();

		//SDL_FreeSurface(surf);
		stbi_image_free(surf);

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


			GLenum texformat = surface_gl_format(channels);
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
	}

	//glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
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
