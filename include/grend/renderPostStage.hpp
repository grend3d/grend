#pragma once

#include <grend/renderFramebuffer.hpp>
#include <grend/glManager.hpp>

namespace grendx {

// XXX: need to pass shader blocks for lighting info, need a cleaner way to do this
using blockParameters = std::map<std::string, std::pair<Buffer::ptr, GLuint>>;

class renderPostStage {
	public:
		typedef std::shared_ptr<renderPostStage> ptr;
		typedef std::weak_ptr<renderPostStage> weakptr;

		renderPostStage(Program::ptr program);
		renderPostStage();

		virtual ~renderPostStage();
		virtual void draw(renderFramebuffer::ptr previous, renderFramebuffer::ptr output);
		virtual void setShader(Program::ptr prog);
		virtual void setUniforms(Shader::parameters& uniforms, blockParameters& uniformBlocks);

	private:
		Program::ptr program = nullptr;
};

renderPostStage::ptr makePostprocessor(Program::ptr program);

// namespace grendx
}
