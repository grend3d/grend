#include <grend/renderFramebuffer.hpp>
#include <grend/utility.hpp>
// for loadPostShader, etc
// those functions could be put into glManager...
#include <grend/engine.hpp>

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

	// TODO: option here
	Shader::parameters nullopts; // XXX: no options
	msaaResolver =
		loadPostShader(GR_PREFIX "shaders/baked/msaa-tonemapped-blit.frag",
		               nullopts);

	if (!msaaResolver) {
		throw std::logic_error("Couldn't load msaa blitter shader");
	}

	msaaResolver->bind();
	msaaResolver->set("samples", int(multisample));
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
	framebuffer->bind();

	allocatedIDs = 0;

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

void renderFramebuffer::resolve(Shader::parameters options) {
#if defined(HAVE_MULTISAMPLE)
	if (!multisample) {
		// regular old framebuffer, nothing to do
		return;
	}

	if (msaaResolver) {
		// custom resolver, for tonemapping in particular
		// TODO: could this be unified with the postprocessing code
		framebuffer->bind();
		msaaResolver->bind();
		DO_ERROR_CHECK();

		glActiveTexture(TEX_GL_SCRATCH);
		colorMultisampled->bind(GL_TEXTURE_2D_MULTISAMPLE);
		msaaResolver->set("colorMS", TEXU_SCRATCH);
		DO_ERROR_CHECK();

		for (auto& [key, val] : options) {
			msaaResolver->set(key, val);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		DO_ERROR_CHECK();
		glDepthMask(GL_TRUE);
		DO_ERROR_CHECK();
		disable(GL_DEPTH_TEST);
		DO_ERROR_CHECK();

		DO_ERROR_CHECK();
		drawScreenquad();

		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebufferMultisampled->obj);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer->obj);
		glBlitFramebuffer(0, 0, width, height,
		                  0, 0, width, height,
		                  GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	} else {
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

		colorMultisampled = genTextureColorMultisample(w, h, multisample,
		                                               rgbaf_if_supported());
		depthMultisampled = genTextureDepthStencilMultisample(w, h, multisample);

		framebufferMultisampled->attach(GL_COLOR_ATTACHMENT0,
		                                colorMultisampled,
		                                GL_TEXTURE_2D_MULTISAMPLE);

		framebufferMultisampled->attach(GL_DEPTH_STENCIL_ATTACHMENT,
		                                depthMultisampled,
		                                GL_TEXTURE_2D_MULTISAMPLE);

		DO_ERROR_CHECK();

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			SDL_Log("Current multisample: %u", multisample);
			SDL_Die("incomplete! (multisampled)");
		}
	}
#endif
}

uint32_t renderFramebuffer::index(float x, float y) {
#if GLSL_VERSION == 100 || GLSL_VERSION == 300
	// can't read from the stencil buffer in gles profiles,
	// could have a fallback picking pass (which could handle more objects anyway)
	return 0xffffffff;
#else

	if (x > width || y > height) {
		return 0xffffffff;
	}

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

	return idx;
#endif
}

uint32_t renderFramebuffer::allocID(void) {
	return ++allocatedIDs;
}
