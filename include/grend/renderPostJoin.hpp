#pragma once

#include <grend/renderPostChain.hpp>

namespace grendx {

class renderPostJoin : public renderPostStage {
	public:
		typedef std::shared_ptr<renderPostJoin> ptr;
		typedef std::weak_ptr<renderPostJoin> weakptr;

		renderPostJoin(Program::ptr prog, renderPostChain::ptr a, renderPostChain::ptr b);

		virtual ~renderPostJoin();

		virtual void setShader(Program::ptr prog);
		virtual void draw(renderFramebuffer::ptr first, renderFramebuffer::ptr output);
		virtual void setUniform(std::string name, Shader::value val);
		virtual void setUniformBlock(std::string name, Buffer::ptr buf, GLuint binding);
		virtual void setUniforms(Shader::parameters& uniforms, blockParameters& uniformBlocks);

		// TODO: probably don't want this to be public, makes setting uniforms easier atm though
		std::array<renderPostChain::ptr, 2> stages;

	private:
		Program::ptr program;

		std::array<renderFramebuffer::ptr, 2> renderBufs = {
			std::make_shared<renderFramebuffer>(128, 128),
			std::make_shared<renderFramebuffer>(128, 128),
		};
};

}
