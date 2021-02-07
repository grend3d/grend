#pragma once

#include <grend/glmIncludes.hpp>
#include <grend/openglIncludes.hpp>
#include <grend/boundingBox.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <tinygltf/tiny_gltf.h>

#include <stdint.h>

namespace grendx {

// TODO: need a material cache, having a way to lazily load/unload texture
//       would be a very good thing
class materialTexture {
	public:
		typedef std::shared_ptr<materialTexture> ptr;
		typedef std::weak_ptr<materialTexture> weakptr;

		materialTexture() { };
		materialTexture(std::string filename);
		void load_texture(std::string filename);
		bool loaded(void) const { return channels != 0; };

		int width = 0, height = 0;
		int channels = 0;
		size_t size;
		std::vector<uint8_t> pixels;
};

struct material {
	typedef std::shared_ptr<material> ptr;
	typedef std::weak_ptr<material>   weakptr;

	enum blend_mode {
		Opaque,
		Mask,
		Blend,
	};

	struct materialFactors {
		glm::vec4 diffuse = {1, 1, 1, 1};
		glm::vec4 ambient = {0, 0, 0, 0};
		glm::vec4 specular = {1, 1, 1, 1};
		glm::vec4 emissive = {0, 0, 0, 1};
		GLfloat   roughness = 1.0;
		// XXX: default metalness factor should be 1.0 for gltf, but
		//      gltf always includes a metalness factor, so use 0.f (no metalness)
		//      for .obj files
		GLfloat   metalness = 0.0;
		GLfloat   opacity = 1.0;
		GLfloat   refract_idx = 1.5;
		enum blend_mode blend = blend_mode::Opaque;
	} factors;

	struct materialMaps {
		materialTexture::ptr diffuse;
		materialTexture::ptr metalRoughness;
		materialTexture::ptr normal;
		materialTexture::ptr ambientOcclusion;
		materialTexture::ptr emissive;
		materialTexture::ptr lightmap;
	} maps;
};

// namespace grendx
}
