#define FRAGMENT_SHADER

precision highp float;
precision mediump sampler2D;
precision mediump samplerCube;

#include <lib/compat.glsl>
#include <lib/constants.glsl>

IN vec3 f_texcoord;
uniform sampler2D skytexture;

vec2 unitDirToLatLong(vec3 dir) {
	return vec2(acos(dir.y),
	            atan(dir.z, dir.x));
}

vec2 latLongToTex(vec2 latlong) {
	return vec2(latlong.y/(2.0*PI),
	            latlong.x/PI);
}

void main(void) {
	vec2 latlong  = unitDirToLatLong(normalize(f_texcoord));
	vec2 texcoord = latLongToTex(latlong);

	FRAG_COLOR = textureLod(skytexture, texcoord, 0.0);

	#if GREND_USE_G_BUFFER
		FRAG_NORMAL      = vec3(0.0, 0.0, 0.0);
		FRAG_POSITION    = vec3(0.0, 0.0, 0.0);
		FRAG_METAL_ROUGH = vec3(0.0, 1.0, 0.0);
		FRAG_RENDER_ID   = 0.0;
	#endif
}
