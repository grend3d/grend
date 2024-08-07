#include <grend/renderPostStage.hpp>
#include <grend/logger.hpp>

using namespace grendx;

renderPostStage::renderPostStage(Program::ptr program) {
	setShader(program);
}

renderPostStage::renderPostStage() {}

renderPostStage::~renderPostStage() {};

void renderPostStage::draw(renderFramebuffer::ptr previous, renderFramebuffer::ptr output) {
	if (!program) {
		// TODO: make 'renderPostStage' an abstract class
		LogError("No shader defined when drawing!");
		return;
	}
	output->bind();
	program->bind();

	glViewport(0, 0, output->width, output->height);
	bindVao(getScreenquadVao());

	glActiveTexture(TEX_GL_SCRATCH);
	previous->color->bind();
	program->set("render_fb", TEXU_SCRATCH);

	program->set("scale_x", previous->scale.x);
	program->set("scale_y", previous->scale.y);
	program->set("rend_x",  (float)previous->width);
	program->set("rend_y",  (float)previous->height);

	glClearColor(0.0, 0.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDepthMask(GL_FALSE);
	disable(GL_DEPTH_TEST);
	DO_ERROR_CHECK();

	program->set("screen_x", (float)output->width);
	program->set("screen_y", (float)output->height);

	DO_ERROR_CHECK();
	drawScreenquad();
}

void renderPostStage::setShader(Program::ptr prog) {
	program = prog;
}

void renderPostStage::setUniforms(Shader::parameters& uniforms,
				                  blockParameters& uniformBlocks)
{
	program->bind();

	for (auto& [key, val] : uniforms) {
		program->set(key, val);
	}

	for (auto& [key, val] : uniformBlocks) {
		auto& [buf, binding] = val;
		program->setUniformBlock(key, buf, binding);
	}
}

// TODO: probably don't need this
// NOTE: assumes the given program is not already linked
renderPostStage::ptr grendx::makePostprocessor(Program::ptr program) {
	program->attribute("v_position", VAO_QUAD_VERTICES);
	program->attribute("v_texcoord", VAO_QUAD_TEXCOORDS);
	program->link();

	return typename renderPostStage::ptr(new renderPostStage(program));
};
