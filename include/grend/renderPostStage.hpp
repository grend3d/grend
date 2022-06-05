#pragma once

#include <grend/renderFramebuffer.hpp>
#include <grend/glManager.hpp>

namespace grendx {

// XXX: need to pass shader blocks for lighting info, need a cleaner way to do this
using blockParameters = std::map<std::string, std::pair<Buffer::ptr, GLuint>>;

class rUninitialized {
	public:
		rUninitialized(unsigned fb_x, unsigned fb_y) {
			// nop
		}

		virtual ~rUninitialized();

		// not an abstract function since it may be good to be able
		// to make a postprocessing stage with no underlying storage
		virtual void setSize(unsigned fb_x, unsigned fb_y) {
			// nop
		}

		// TODO: maybe make renderFramebuffer a base class, and have a
		//       derived renderFramebufferStencil or something so
		//       renderFramebuffer could be used here
		Framebuffer::ptr framebuffer;
		unsigned width = 0;
		unsigned height = 0;

		Texture::ptr renderTexture;
};

class rOutput : public rUninitialized {
	public:
		rOutput(unsigned fb_x, unsigned fb_y) : rUninitialized(fb_x, fb_y) {
			// XXX: default constructor for Framebuffer refers to the
			//      default opengl framebuffer, maybe should have
			//      a getter in gl_manager to avoid littering default FB
			//      pointers
			framebuffer = Framebuffer::ptr(new Framebuffer());
			setSize(fb_x, fb_y);
		}
		virtual ~rOutput();

		virtual void setSize(unsigned fb_x, unsigned fb_y) {
			width = fb_x;
			height = fb_y;
		}
};

class rStage : public rUninitialized {
	public:
		rStage(unsigned fb_x, unsigned fb_y) : rUninitialized(fb_x, fb_y) {
			setSize(fb_x, fb_y);
		}
		virtual ~rStage();

		virtual void setSize(unsigned fb_x, unsigned fb_y) {
			if (fb_x != width || fb_y != height) {
				framebuffer = genFramebuffer();
				framebuffer->bind();
				renderTexture =
					framebuffer->attach(GL_COLOR_ATTACHMENT0,
						genTextureFormat(fb_x, fb_y, rgbaf_if_supported()));

				width = fb_x;
				height = fb_y;
			}
		}
};

template <class Storage>
class renderPostStage : public Storage {
	public:
		typedef std::shared_ptr<renderPostStage<Storage>> ptr;
		typedef std::weak_ptr<renderPostStage<Storage>> weakptr;

		renderPostStage(Program::ptr program, unsigned fb_x, unsigned fb_y)
			: Storage(fb_x, fb_y)
		{
			setShader(program);
		}
		virtual ~renderPostStage() {};

		void draw(void) {
			draw(nullptr, nullptr);
		}

		// TODO: hmmm... can't do this with gbuffers...
		//       maybe a struct holding all gbuffer textures?
		void draw(Texture::ptr previous, Texture::ptr depth) {
			this->framebuffer->bind();
			program->bind();
			glViewport(0, 0, this->width, this->height);

			bindVao(getScreenquadVao());

			if (previous) {
				glActiveTexture(TEX_GL_SCRATCH);
				previous->bind();
				program->set("render_fb", TEXU_SCRATCH);
			}

			/*
			if (depth) {
				glActiveTexture(TEX_GL_SCRATCHB);
				depth->bind();
				program->set("render_depth", TEXU_SCRATCHB);
			}
			*/

			// No scaling for previous postprocessing stages, although that
			// could be useful for doing expensive effects at reduced resolution
			program->set("scale_x", 1.f);
			program->set("scale_y", 1.f);
			//
			program->set("rend_x", (float)this->width);
			program->set("rend_y", (float)this->height);

			do_draw();
		}

		void draw(renderFramebuffer::ptr previous) {
			this->framebuffer->bind();
			program->bind();

			glViewport(0, 0, this->width, this->height);
			bindVao(getScreenquadVao());

			glActiveTexture(TEX_GL_SCRATCH);
			previous->color->bind();
			program->set("render_fb", TEXU_SCRATCH);

			/*
			glActiveTexture(TEX_GL_SCRATCHB);
			previous->depth->bind();
			program->set("render_depth", TEXU_SCRATCHB);
			*/

			program->set("scale_x", previous->scale.x);
			program->set("scale_y", previous->scale.y);
			program->set("rend_x",  (float)previous->width);
			program->set("rend_y",  (float)previous->height);

			do_draw();
		}

		void setShader(Program::ptr prog) {
			program = prog;
		}

		void setUniforms(Shader::parameters& uniforms,
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

		Program::ptr program;

	private:
		// called after binding everything from one of the draw() functions
		void do_draw(void) {
			glClearColor(0.0, 0.0, 1.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glDepthMask(GL_FALSE);
			disable(GL_DEPTH_TEST);
			DO_ERROR_CHECK();

			program->set("screen_x", (float)this->width);
			program->set("screen_y", (float)this->height);

			DO_ERROR_CHECK();
			drawScreenquad();
		}
};

class renderPostChain {
	public:
		typedef std::shared_ptr<renderPostChain> ptr;
		typedef std::weak_ptr<renderPostChain> weakptr;

		renderPostChain(std::vector<Program::ptr> stages,
		                unsigned fb_x,
		                unsigned fb_y)
		{
			if (stages.size() == 0) {
				// TODO: could the fancy new c++ concepts/contraints do this?
				throw std::logic_error("renderPostChain(): need at least one stage");
			}

			for (auto it = stages.begin(); it != stages.end(); it++) {
				if (std::next(it) == stages.end()) {
					output =
						std::make_shared<renderPostStage<rOutput>>(*it, fb_x, fb_y);
					break;

				} else {
					renderPostStage<rStage>::ptr stage =
						std::make_shared<renderPostStage<rStage>>(*it, fb_x, fb_y);
					prestages.push_back(stage);
				}
			}
		}

		void draw(renderFramebuffer::ptr fb) {
			fb->resolve(uniforms);
			Texture::ptr current = fb->color;

			#if GREND_USE_G_BUFFER
				// TODO: clearer texture unit defines for these
				glActiveTexture(TEX_GL_NORMAL);
				fb->normal->bind();
				//program->set("normal_fb", TEXU_NORMAL);
				setUniform("normal_fb", TEXU_NORMAL);

				glActiveTexture(TEX_GL_DIFFUSE);
				fb->position->bind();
				//program->set("position_fb", TEXU_SKYBOX);
				setUniform("position_fb", TEXU_DIFFUSE);

				glActiveTexture(TEX_GL_METALROUGH);
				fb->metalRoughness->bind();
				setUniform("metalroughness_fb", TEXU_METALROUGH);

				glActiveTexture(TEX_GL_AO);
				fb->renderID->bind();
				setUniform("renderID_fb", TEXU_AO);
			#endif

			glActiveTexture(TEX_GL_SCRATCHB);
			fb->depth->bind();
			setUniform("render_depth", TEXU_SCRATCHB);

			for (auto& stage : prestages) {
				stage->setUniforms(uniforms, uniformBlocks);
				stage->draw(current, fb->depth);
				current = stage->renderTexture;
			}

			output->setUniforms(uniforms, uniformBlocks);
			output->draw(current, fb->depth);
		}

		void setSize(unsigned fb_x, unsigned fb_y) {
			for (auto& stage : prestages) {
				stage->setSize(fb_x, fb_y);
			}

			output->setSize(fb_x, fb_y);
		}

		void setUniform(std::string name, Shader::value val) {
			uniforms[name] = val;
		}

		void setUniformBlock(std::string name, Buffer::ptr buf, GLuint binding) {
			uniformBlocks[name] = {buf, binding};
		}

	private:
		std::vector<renderPostStage<rStage>::ptr> prestages;
		renderPostStage<rOutput>::ptr output;
		Shader::parameters uniforms;
		blockParameters uniformBlocks;
};

// NOTE: assumes the given program is not already linked
template <class Storage>
typename renderPostStage<Storage>::ptr
makePostprocessor(Program::ptr program, unsigned fb_x, unsigned fb_y) {
	program->attribute("v_position", VAO_QUAD_VERTICES);
	program->attribute("v_texcoord", VAO_QUAD_TEXCOORDS);
	program->link();

	return typename renderPostStage<Storage>::ptr(new renderPostStage<Storage>(program, fb_x, fb_y));
};

// namespace grendx
}
