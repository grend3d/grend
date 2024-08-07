#include <grend/renderPostJoin.hpp>

using namespace grendx;

renderPostJoin::renderPostJoin(Program::ptr prog, renderPostChain::ptr a, renderPostChain::ptr b)
	: program(prog)
{
	stages[0] = a;
	stages[1] = b;
}

renderPostJoin::~renderPostJoin() {};

void renderPostJoin::setShader(Program::ptr prog) {
	program = prog;
}

void renderPostJoin::draw(renderFramebuffer::ptr first, renderFramebuffer::ptr output) {
	for (auto& buf : renderBufs) {
		if (buf->width != output->width
		 || buf->height != output->height)
		{
			// sync render buffer size with output buffer size
			buf->setSize(output->width, output->height);
		}
	}

	// draw each render chain
	for (size_t i = 0; i < stages.size(); i++) {
		stages[i]->draw(first, renderBufs[i]);
	}

	// join with shader
	program->bind();
	output->bind();

	glActiveTexture(TEX_GL_SCRATCH);
	renderBufs[0]->color->bind();
	program->set("chain1", TEXU_SCRATCH);

	glActiveTexture(TEX_GL_SCRATCHB);
	renderBufs[1]->color->bind();
	program->set("chain2", TEXU_SCRATCHB);

	program->set("screen_x", (float)output->width);
	program->set("screen_y", (float)output->height);

	DO_ERROR_CHECK();
	drawScreenquad();
}

void renderPostJoin::setUniform(std::string name, Shader::value val) {
	for (size_t i = 0; i < stages.size(); i++) {
		stages[i]->setUniform(name, val);
	}
}

void renderPostJoin::setUniformBlock(std::string name, Buffer::ptr buf, GLuint binding) {
	for (size_t i = 0; i < stages.size(); i++) {
		stages[i]->setUniformBlock(name, buf, binding);
	}
}

void renderPostJoin::setUniforms(Shader::parameters& uniforms,
				                 blockParameters& uniformBlocks)
{
	for (auto& [key, val] : uniforms) {
		setUniform(key, val);
	}

	for (auto& [key, val] : uniformBlocks) {
		auto& [buf, binding] = val;
		setUniformBlock(key, buf, binding);
	}
}
