#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;

#include <lib/postprocessing-uniforms.glsl>
#include <lib/compat.glsl>
#include <lib/tonemapping.glsl>

IN vec2 f_texcoord;

#if GLSL_VERSION == 100 || GLSL_VERSION == 300
void main(void) {
	// nop
}
#else
uniform int samples;
uniform sampler2DMS colorMS;

// TODO: MSAA with G-buffers might not make a lot of sense, just
//       want to experiment with it
//       (particularly to see if it gives better results with temporal upscaling)
#if GREND_USE_G_BUFFER
	uniform sampler2DMS normalMS;
	uniform sampler2DMS positionMS;
	uniform sampler2DMS renderIDMS;
#endif

void main(void) {
	ivec2 size = textureSize(colorMS);
	ivec2 uv   = ivec2(size*f_texcoord);

	vec4 color = vec4(0);

	for (int i = 0; i < samples; i++) {
		vec4 samp = texelFetch(colorMS, uv, i);
		color += doTonemap(samp, exposure);
	}

	FRAG_COLOR = color / float(samples);
	#if GREND_USE_G_BUFFER
		// don't average G-buffer info, pass along first sample
		FRAG_NORMAL    = texelFetch(normalMS,   uv, 0).rgb;
		FRAG_POSITION  = texelFetch(positionMS, uv, 0).rgb;
		FRAG_RENDER_ID = texelFetch(renderIDMS, uv, 0).r;
	#endif
}
#endif
