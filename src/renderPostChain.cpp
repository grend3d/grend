#include <grend/renderPostChain.hpp>

using namespace grendx;

renderPostChain::renderPostChain() {}

renderPostChain::renderPostChain(std::vector<Program::ptr> stages) {
	if (stages.size() == 0) {
		// TODO: could the fancy new c++ concepts/contraints do this?
		throw std::logic_error("renderPostChain(): need at least one stage");
	}

	for (auto& prog : stages) {
		this->stages.push_back(std::make_shared<renderPostStage>(prog));
	}
}

void renderPostChain::add(Program::ptr prog) {
	this->stages.push_back(std::make_shared<renderPostStage>(prog));
}

void renderPostChain::add(renderPostStage::ptr stage) {
	this->stages.push_back(stage);
}

void renderPostChain::draw(renderFramebuffer::ptr first, renderFramebuffer::ptr output) {
	// TODO: should resolve somewhere else before calling this
	first->resolve(uniforms);

	#if GREND_USE_G_BUFFER
		// TODO: clearer texture unit defines for these
		glActiveTexture(TEX_GL_NORMAL);
		first->normal->bind();
		//program->set("normal_fb", TEXU_NORMAL);
		setUniform("normal_fb", TEXU_NORMAL);

		glActiveTexture(TEX_GL_DIFFUSE);
		first->position->bind();
		//program->set("position_fb", TEXU_SKYBOX);
		setUniform("position_fb", TEXU_DIFFUSE);

		glActiveTexture(TEX_GL_METALROUGH);
		first->metalRoughness->bind();
		setUniform("metalroughness_fb", TEXU_METALROUGH);

		glActiveTexture(TEX_GL_AO);
		first->renderID->bind();
		setUniform("renderID_fb", TEXU_AO);
	#endif

	glActiveTexture(TEX_GL_SCRATCHB);
	first->depth->bind();
	setUniform("render_depth", TEXU_SCRATCHB);

	int targetWidth  = output->width*scaleX;
	int targetHeight = output->height*scaleY;

	for (auto& buf : renderBufs) {
		if (buf->width != targetWidth
		 || buf->height != targetHeight)
		{
			// sync render buffer size with output buffer size
			buf->setSize(targetWidth, targetHeight);
		}
	}

	for (size_t i = 0; i < stages.size(); i++) {
		auto last = (i == 0)?                 first  : renderBufs[(i - 1) & 1];
		auto cur  = (i == stages.size() - 1)? output : renderBufs[i & 1];

		stages[i]->setUniforms(uniforms, uniformBlocks);
		stages[i]->draw(last, cur);
	}
}

void renderPostChain::setUniform(std::string name, Shader::value val) {
	uniforms[name] = val;
}

void renderPostChain::setUniformBlock(std::string name, Buffer::ptr buf, GLuint binding) {
	uniformBlocks[name] = {buf, binding};
}

void renderPostChain::setScale(float x, float y) {
	scaleX = x;
	scaleY = y;
}
