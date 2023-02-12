#pragma once

#include <grend/glmIncludes.hpp>
#include <grend/openglIncludes.hpp>
#include <grend/textureData.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include <stdint.h>

namespace grendx {

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

		GLfloat   roughness      = 1.0;
		// XXX: default metalness factor should be 1.0 for gltf, but
		//      gltf always includes a metalness factor, so use 0.f (no metalness)
		//      for .obj files
		GLfloat   metalness      = 0.0;
		GLfloat   opacity        = 1.0;
		GLfloat   alphaCutoff    = 0.5;
		GLfloat   refract_idx    = 1.5;
		enum blend_mode blend = blend_mode::Opaque;
	} factors;

	struct materialMaps {
		textureData::ptr diffuse;
		textureData::ptr metalRoughness;
		textureData::ptr normal;
		textureData::ptr ambientOcclusion;
		textureData::ptr emissive;
		textureData::ptr lightmap;
	} maps;
};

// namespace grendx
}
