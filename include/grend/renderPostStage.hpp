#pragma once

#include <grend/renderFramebuffer.hpp>
#include <grend/gl_manager.hpp>

namespace grendx {

class rUninitialized {
	public:
		rUninitialized(unsigned fb_x, unsigned fb_y) {
			// nop
		}

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

		virtual void setSize(unsigned fb_x, unsigned fb_y) {
			framebuffer = gen_framebuffer();
			framebuffer->bind();
			renderTexture = framebuffer->attach(GL_COLOR_ATTACHMENT0,
			                    gen_texture_color(fb_x, fb_y, GL_RGBA16F));

			width = fb_x;
			height = fb_y;
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

		void draw(void) {
			draw(nullptr, nullptr);
		}

		void draw(Texture::ptr previous, Texture::ptr depth) {
			this->framebuffer->bind();
			program->bind();

			bind_vao(get_screenquad_vao());

			if (previous) {
				glActiveTexture(GL_TEXTURE6);
				previous->bind();
				program->set("render_fb", 6);
			}

			if (depth) {
				glActiveTexture(GL_TEXTURE7);
				depth->bind();
				program->set("render_depth", 7);
			}

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

			bind_vao(get_screenquad_vao());

			glActiveTexture(GL_TEXTURE6);
			previous->color->bind();
			program->set("render_fb", 6);

			glActiveTexture(GL_TEXTURE7);
			previous->depth->bind();
			program->set("render_depth", 7);

			program->set("scale_x", previous->scale.x);
			program->set("scale_y", previous->scale.y);
			program->set("rend_x",  (float)previous->width);
			program->set("rend_y",  (float)previous->height);

			do_draw();
		}

		void setShader(Program::ptr prog) {
			program = prog;
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
			// TODO: parameters
			program->set("exposure", 1.f);

			DO_ERROR_CHECK();
			draw_screenquad();
		}
};

// NOTE: assumes the given program is not already linked
template <class Storage>
typename renderPostStage<Storage>::ptr
makePostprocessor(Program::ptr program, unsigned fb_x, unsigned fb_y) {
	bind_vao(get_screenquad_vao());
	glBindAttribLocation(program->obj, 0, "v_position");
	glBindAttribLocation(program->obj, 1, "v_texcoord");
	DO_ERROR_CHECK();

	link_program(program);

	return typename renderPostStage<Storage>::ptr(new renderPostStage<Storage>(program, fb_x, fb_y));
};

// namespace grendx
}