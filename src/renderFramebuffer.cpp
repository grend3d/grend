#include <grend/renderFramebuffer.hpp>
#include <grend/utility.hpp>

using namespace grendx;

renderFramebuffer::renderFramebuffer(int Width, int Height, unsigned Multisample)
	: width(Width), height(Height), multisample(Multisample)
{
	framebuffer = genFramebuffer();

#if defined(HAVE_MULTISAMPLE)
	if (multisample) {
		std::cerr << "Multisample! samples=" << multisample << std::endl;
		framebufferMultisampled = genFramebuffer();
	}
#endif

	setSize(Width, Height);
}

renderFramebuffer::renderFramebuffer(Framebuffer::ptr fb, int Width, int Height)
	: width(Width), height(Height)
{
	framebuffer = fb;
}

void renderFramebuffer::bind(void) {
#if defined(HAVE_MULTISAMPLE)
	if (multisample) {
		framebufferMultisampled->bind();
	} else {
		framebuffer->bind();
	}
#else
	framebuffer->bind();
#endif
}

void renderFramebuffer::clear(void) {
	drawn_meshes.clear();

	framebuffer->bind();
	glClearStencil(0);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	DO_ERROR_CHECK();

#if defined(HAVE_MULTISAMPLE)
	if (multisample) {
		framebufferMultisampled->bind();

		glClearStencil(0);
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		DO_ERROR_CHECK();
	}
#endif
}

void renderFramebuffer::resolve(void) {
#if defined(HAVE_MULTISAMPLE)
	if (multisample) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferMultisampled->obj);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->obj);
		glBlitFramebuffer(0, 0, width, height,
		                  0, 0, width, height,
		                  GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT,
		                  GL_NEAREST);
	}
#endif
}

void renderFramebuffer::setSize(int Width, int Height) {
	auto w = width  = max(1, Width);
	auto h = height = max(1, Height);

	framebuffer->bind();
	color = framebuffer->attach(GL_COLOR_ATTACHMENT0,
			genTextureColor(w, h, rgbaf_if_supported()));
	depth = framebuffer->attach(GL_DEPTH_STENCIL_ATTACHMENT,
			genTextureDepthStencil(w, h));
	DO_ERROR_CHECK();

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		SDL_Die("incomplete! (plain)");
	}

#if defined(HAVE_MULTISAMPLE)
	if (multisample) {
		framebufferMultisampled->bind();

		framebufferMultisampled->attach(GL_COLOR_ATTACHMENT0,
				genTextureColorMultisample(w, h, multisample, rgbaf_if_supported()),
				GL_TEXTURE_2D_MULTISAMPLE);
		framebufferMultisampled->attach(GL_DEPTH_STENCIL_ATTACHMENT,
			genTextureDepthStencilMultisample(w, h, multisample),
			GL_TEXTURE_2D_MULTISAMPLE);

		DO_ERROR_CHECK();

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			SDL_Log("Current multisample: %u", multisample);
			SDL_Die("incomplete! (multisampled)");
		}
	}
#endif
}

gameMesh::ptr renderFramebuffer::index(float x, float y) {
#if GLSL_VERSION == 100 || GLSL_VERSION == 300
	// can't read from the stencil buffer in gles profiles,
	// could have a fallback picking pass (which could handle more objects anyway)
	return nullptr;
#else

	// TODO: use these at some point
	//GLbyte color[4];
	//GLfloat depth;
	GLubyte idx;

	// adjust coordinates for resolution scaling
	unsigned adx = width * x * scale.x;
	unsigned ady = (height - height*y - 1) * scale.y;

	framebuffer->bind();
	//glReadPixels(adx, ady, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color);
	//glReadPixels(adx, ady, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	glReadPixels(adx, ady, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &idx);
	DO_ERROR_CHECK();

	return index(idx);
#endif
}

gameMesh::ptr renderFramebuffer::index(unsigned idx) {
	if (idx > 0 && idx <= drawn_meshes.size()) {
		puts("got here");
		return drawn_meshes[idx - 1];
	}

	return nullptr;
}
