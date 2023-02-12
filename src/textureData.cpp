#include <grend/textureData.hpp>
#include <grend/logger.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

using namespace grendx;

textureData::textureData(std::string filename) {
	load_texture(filename);
}

bool textureData::load_texture(std::string filename) {
	if (stbi_is_hdr(filename.c_str())) {
		// load image components as floats
		LogFmt("Loading {} (float)", filename);
		float *datas = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);

		if (!datas) {
			throw std::logic_error("Couldn't load texture");
			return false;
		}

		size_t imgsize = width * height * channels;
		std::vector<float> px;

		px.insert(px.end(), datas, datas + imgsize);
		this->pixels = std::move(px);
		stbi_image_free(datas);

		return true;

	} else if (stbi_is_16_bit(filename.c_str())) {
		// load image components as 16 bit uints
		LogFmt("Loading {} (16 bit)", filename);
		uint16_t *datas = stbi_load_16(filename.c_str(), &width, &height, &channels, 0);

		if (!datas) {
			throw std::logic_error("Couldn't load texture");
			return false;
		}

		size_t imgsize = width * height * channels;
		std::vector<uint16_t> px;

		px.insert(px.end(), datas, datas + imgsize);
		this->pixels = std::move(px);
		stbi_image_free(datas);

		return true;

	} else {
		// otherwise assume components are regular 8 bit uints
		// TODO: handle compressed textures transparently here
		LogFmt("Loading {} (8 bit)", filename);
		uint8_t *datas = stbi_load(filename.c_str(), &width, &height, &channels, 0);

		if (!datas) {
			throw std::logic_error("Couldn't load texture");
			return false;
		}

		size_t imgsize = width * height * channels;
		std::vector<uint8_t> px;

		px.insert(px.end(), datas, datas + imgsize);

		this->pixels = std::move(px);
		stbi_image_free(datas);

		return true;
	}
}

