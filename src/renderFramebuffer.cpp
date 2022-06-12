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

#if GREND_USE_G_BUFFER
	indexBuffer = genFramebuffer();
#endif

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

	#if GREND_USE_G_BUFFER
	/*
		const unsigned bufs[] = {
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
		};
		glDrawBuffers(4, bufs);
		*/
	#endif
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
		unsigned bufs[] = {
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,
			GL_COLOR_ATTACHMENT4,
		};

		// custom resolver, for tonemapping in particular
		// TODO: could this be unified with the postprocessing code
		framebuffer->bind();
		msaaResolver->bind();
		DO_ERROR_CHECK();

		glDrawBuffers(5, bufs);

		glActiveTexture(TEX_GL_SCRATCH);
		colorMultisampled->bind(GL_TEXTURE_2D_MULTISAMPLE);
		msaaResolver->set("colorMS", TEXU_SCRATCH);

#if GREND_USE_G_BUFFER
		glActiveTexture(TEX_GL_NORMAL);
		normalMultisampled->bind(GL_TEXTURE_2D_MULTISAMPLE);
		msaaResolver->set("normalMS", TEXU_NORMAL);

		glActiveTexture(TEX_GL_SCRATCHB);
		positionMultisampled->bind(GL_TEXTURE_2D_MULTISAMPLE);
		msaaResolver->set("positionMS", TEXU_SCRATCHB);

		glActiveTexture(TEX_GL_METALROUGH);
		metalRoughnessMultisampled->bind(GL_TEXTURE_2D_MULTISAMPLE);
		msaaResolver->set("metalRoughnessMS", TEXU_METALROUGH);

		glActiveTexture(TEX_GL_LIGHTMAP);
		renderIDMultisampled->bind(GL_TEXTURE_2D_MULTISAMPLE);
		msaaResolver->set("renderIDMS", TEXU_LIGHTMAP);
#endif
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

		drawScreenquad();
		DO_ERROR_CHECK();

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
	SDL_Log("Allocating internal buffer with dimensions (%u, %u), MSAA: %u",
	        Width, Height, multisample);

	auto w = width  = max(1, Width);
	auto h = height = max(1, Height);

	framebuffer->bind();

	color = genTextureFormat(w, h, rgbaf_if_supported());
	depth = genTextureFormat(w, h, depth_stencil_format());

	framebuffer->attach(GL_COLOR_ATTACHMENT0,        color);
	framebuffer->attach(GL_DEPTH_STENCIL_ATTACHMENT, depth);

#if GREND_USE_G_BUFFER
	normal         = genTextureFormat(w, h, rgbf_if_supported());
	position       = genTextureFormat(w, h, rgbf_if_supported());
	metalRoughness = genTextureFormat(w, h, rgbf_if_supported());
	DO_ERROR_CHECK();
	renderID       = genTextureFormat(w, h, index_format());
	DO_ERROR_CHECK();

	framebuffer->attach(GL_COLOR_ATTACHMENT1, normal);
	framebuffer->attach(GL_COLOR_ATTACHMENT2, position);
	framebuffer->attach(GL_COLOR_ATTACHMENT3, metalRoughness);
	framebuffer->attach(GL_COLOR_ATTACHMENT4, renderID);
	DO_ERROR_CHECK();

	const unsigned bufs[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4,
	};

	glDrawBuffers(5, bufs);

	// TODO: Could split renderIDs into seperate option, probably not though
	indexBuffer->bind();
	indexBuffer->attach(GL_COLOR_ATTACHMENT0, renderID);

	framebuffer->bind();
#endif

	DO_ERROR_CHECK();

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		SDL_Die("incomplete! (plain)");
	}

#if defined(HAVE_MULTISAMPLE)
	if (multisample) {
		framebufferMultisampled->bind();

		auto gentex = [&](const glTexFormat& format) {
			// make big function call less verbose
			return genTextureFormatMS(w, h, multisample, format);
		};

		auto attach = [&](GLenum attachment, Texture::ptr tex) {
			return framebufferMultisampled->attach(attachment,
			                                       tex,
			                                       GL_TEXTURE_2D_MULTISAMPLE);
		};

		colorMultisampled = gentex(rgbaf_if_supported());
		depthMultisampled = gentex(depth_stencil_format());

		attach(GL_COLOR_ATTACHMENT0,        colorMultisampled);
		attach(GL_DEPTH_STENCIL_ATTACHMENT, depthMultisampled);

		#if GREND_USE_G_BUFFER
			normalMultisampled         = gentex(rgbf_if_supported());
			positionMultisampled       = gentex(rgbf_if_supported());
			metalRoughnessMultisampled = gentex(rgbf_if_supported());
			renderIDMultisampled       = gentex(index_format());

			attach(GL_COLOR_ATTACHMENT1, normalMultisampled);
			attach(GL_COLOR_ATTACHMENT2, positionMultisampled);
			attach(GL_COLOR_ATTACHMENT3, metalRoughnessMultisampled);
			attach(GL_COLOR_ATTACHMENT4, renderIDMultisampled);

			glDrawBuffers(5, bufs);
		#endif


		DO_ERROR_CHECK();

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			SDL_Log("Current multisample: %u", multisample);
			SDL_Die("incomplete! (multisampled)");
		}
	}
#endif
}

uint32_t renderFramebuffer::index(float x, float y) {
#if GREND_USE_G_BUFFER
	if (x > width || y > height) {
		return 0xffffffff;
	}

	uint32_t idx = 0xffffffff;
	indexBuffer->bind();

	// adjust coordinates for resolution scaling
	unsigned adx = width * x * scale.x;
	unsigned ady = (height - height*y - 1) * scale.y;

	#if INDEX_FORMAT_BITS == 8
		GLubyte colors[4];
		glReadPixels(adx, ady, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &colors);
		idx = colors[0];

	#elif INDEX_FORMAT_BITS == 16
		GLushort idxshort;
		glReadPixels(adx, ady, 1, 1, GL_RED, GL_UNSIGNED_SHORT, &idxshort);
		idx = idxshort;
	#else
		#warning "Unimplemented indexer for requested index format"
	#endif

	DO_ERROR_CHECK();
	SDL_Log("have sample: %u\n", idx);
	return idx;

#else
	return 0xffffffff;
#endif
}

uint32_t renderFramebuffer::allocID(void) {
	return ++allocatedIDs;
}
