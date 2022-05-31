#pragma once

#include <grend/glmIncludes.hpp>
#include <grend/openglIncludes.hpp>
#include <grend/boundingBox.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
//#include <tinygltf/tiny_gltf.h>

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

		// XXX: could use GL enums directly here, seems like that might
		//      be tying it too closely to the OpenGL api though... idk,
		//      if this is too unwieldy can always remove it later
		enum filter {
			// only 'nearest' and 'linear' valid for mag filter
			// can throw away mipmap specification on the opengl side,
			// if that that's specified for a mag filter
			Nearest,
			Linear,

			NearestMipmaps, // not used as mode
			NearestMipmapNearest,
			NearestMipmapLinear,

			LinearMipmaps,  // not used as mode
			LinearMipmapNearest,
			LinearMipmapLinear,
		};

		enum wrap {
			ClampToEdge,
			MirroredRepeat,
			Repeat,
		};

		enum imageType {
			Plain,
			Etc2Compressed,
			VecTex,
		};

		// higher quality defaults, tweakable settings can be done in loading
		enum filter minFilter = filter::LinearMipmapLinear;
		enum filter magFilter = filter::Linear;

		enum wrap wrapS = wrap::Repeat;
		enum wrap wrapT = wrap::Repeat;

		enum imageType type = imageType::Plain;
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
