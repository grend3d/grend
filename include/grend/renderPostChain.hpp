#pragma once

#include <grend/renderFramebuffer.hpp>
#include <grend/renderPostStage.hpp>
#include <grend/glManager.hpp>

#include <array>

namespace grendx {

class renderPostChain {
	public:
		typedef std::shared_ptr<renderPostChain> ptr;
		typedef std::weak_ptr<renderPostChain> weakptr;

		renderPostChain();
		renderPostChain(std::vector<Program::ptr> stages);

		void add(Program::ptr prog);
		void add(renderPostStage::ptr stage);
		void draw(renderFramebuffer::ptr first, renderFramebuffer::ptr output);
		void setUniform(std::string name, Shader::value val);
		void setUniformBlock(std::string name, Buffer::ptr buf, GLuint binding);

		void setScale(float x, float y);

	private:
		std::vector<renderPostStage::ptr> stages;
		Shader::parameters uniforms;
		blockParameters uniformBlocks;

		std::array<renderFramebuffer::ptr, 2> renderBufs = {
			std::make_shared<renderFramebuffer>(128, 128),
			std::make_shared<renderFramebuffer>(128, 128),
		};

		float scaleX = 1;
		float scaleY = 1;
};

// namespace grendx
}
