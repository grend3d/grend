#include <grend/textureData.hpp>
#include <grend/logger.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

using namespace grendx;

textureData::textureData(std::string filename) {
	load_texture(filename);
}

void textureData::load_texture(std::string filename) {
	LogFmt("loading {}", filename);
	uint8_t *datas = stbi_load(filename.c_str(), &width,
	                           &height, &channels, 0);

	if (!datas) {
		throw std::logic_error("Couldn't load material texture " + filename);
	}

	size_t imgsize = width * height * channels;
	//pixels.insert(pixels.end(), datas, datas + imgsize);
	for (unsigned i = 0; i < imgsize; i++) {
		pixels.push_back(datas[i]);
	}

	stbi_image_free(datas);
}

