#pragma once

#include <grend/sceneNode.hpp>
#include <grend/sceneModel.hpp>
#include <grend/glManager.hpp>

#include <vector>
#include <stdint.h>

namespace grendx {

class renderFramebuffer {
	uint32_t allocatedIDs = 0;

	public:
		typedef std::shared_ptr<renderFramebuffer> ptr;
		typedef std::weak_ptr<renderFramebuffer> weakptr;

		// TODO: Have like 3 different conventions for this scattered through
		//       the codebase, need to just pick one
		renderFramebuffer(int Width, int Height, unsigned multisample = 0);
		renderFramebuffer(Framebuffer::ptr fb, int Width, int Height);

		void bind(void);
		void clear(void);
		// XXX: default parameters, need to pass uniforms for tonemapping
		// TODO: reference/optional, avoid copying
		void resolve(Shader::parameters options = {});
		void setSize(int Width, int Height);
		uint32_t index(float x, float y);
		uint32_t allocID(void);

		Framebuffer::ptr framebuffer;
		Texture::ptr color;
		Texture::ptr depth;

		#if defined(HAVE_MULTISAMPLE)
			Framebuffer::ptr framebufferMultisampled;
			Texture::ptr colorMultisampled;
			Texture::ptr depthMultisampled;
		#endif

#if GLSL_VERSION >= 300 && GREND_USE_G_BUFFER
		Texture::ptr normal;
		Texture::ptr metalRoughness;
		Texture::ptr position;
		Texture::ptr renderID;
		Framebuffer::ptr indexBuffer;

		// TODO: probably won't ever use multisampling with G-buffers
		//       in anything serious, just for experimentation/comparison,
		//       the memory cost of these full-resolution multisampled
		//       layers is huge
		#if defined(HAVE_MULTISAMPLE)
			Texture::ptr normalMultisampled;
			Texture::ptr positionMultisampled;
			Texture::ptr metalRoughnessMultisampled;
			Texture::ptr renderIDMultisampled;;
		#endif
#endif

		// shader to blit multisampled buffer to a normal framebuffer
		Program::ptr msaaResolver = nullptr;

		int width = -1, height = -1;
		unsigned multisample = 0;

		struct {
			float x, y;
			float min_x, min_y;
		} scale = {
			1.0, 1.0,
			0.5, 0.5,
		};
};

// namespace grendx;
}
