#include <grend/textureData.hpp>
#include <grend/logger.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

using namespace grendx;

textureData::textureData(const std::string& filename, bool flipVertical) {
	load_texture(filename, flipVertical);
}

bool textureData::load_texture(const std::string& filename, bool flipVertical) {
	stbi_set_flip_vertically_on_load(flipVertical);

	if (stbi_is_hdr(filename.c_str())) {
		// load image components as floats
		LogFmt("Loading {} (float)", filename);
		float *datas = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);

		if (!datas) {
			stbi_set_flip_vertically_on_load(false);
			throw std::logic_error("Couldn't load texture");
			return false;
		}

		size_t imgsize = width * height * channels;
		std::vector<float> px;

		px.insert(px.end(), datas, datas + imgsize);
		this->pixels = std::move(px);
		stbi_image_free(datas);

		stbi_set_flip_vertically_on_load(false);
		return true;

	} else if (stbi_is_16_bit(filename.c_str())) {
		// load image components as 16 bit uints
		LogFmt("Loading {} (16 bit)", filename);
		uint16_t *datas = stbi_load_16(filename.c_str(), &width, &height, &channels, 0);

		if (!datas) {
			stbi_set_flip_vertically_on_load(false);
			throw std::logic_error("Couldn't load texture");
			return false;
		}

		size_t imgsize = width * height * channels;
		std::vector<uint16_t> px;

		px.insert(px.end(), datas, datas + imgsize);
		this->pixels = std::move(px);
		stbi_image_free(datas);

		stbi_set_flip_vertically_on_load(false);
		return true;

	} else {
		// otherwise assume components are regular 8 bit uints
		// TODO: handle compressed textures transparently here
		LogFmt("Loading {} (8 bit)", filename);
		uint8_t *datas = stbi_load(filename.c_str(), &width, &height, &channels, 0);

		if (!datas) {
			stbi_set_flip_vertically_on_load(false);
			throw std::logic_error("Couldn't load texture");
			return false;
		}

		size_t imgsize = width * height * channels;
		std::vector<uint8_t> px;

		px.insert(px.end(), datas, datas + imgsize);

		this->pixels = std::move(px);
		stbi_image_free(datas);

		stbi_set_flip_vertically_on_load(false);
		return true;
	}
}

