#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>

IN vec3 f_texcoord;
uniform samplerCube skytexture;

void main(void) {
	//gl_FragColor = textureCube(skytexture, f_texcoord);
	//gl_FragColor = textureLod(skytexture, f_texcoord, 0);
	FRAG_COLOR = textureLod(skytexture, f_texcoord, 0.0);

	#if GREND_USE_G_BUFFER
		FRAG_NORMAL      = vec3(0.0, 0.0, 0.0);
		FRAG_POSITION    = vec3(0.0, 0.0, 0.0);
		FRAG_METAL_ROUGH = vec3(0.0, 1.0, 0.0);
		FRAG_RENDER_ID   = 0.0;
	#endif
}
