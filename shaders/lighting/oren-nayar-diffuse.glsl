#pragma once
#include <lib/utility.glsl>

float oren_nayar_diffuse(float rough, vec3 L, vec3 N, vec3 V) {
	float thetai = acos(dot(N, L));
	float thetar = acos(dot(N, V));
	float alpha = max(thetai, thetar);
	float beta  = min(thetai, thetar);
	float sigma2 = rough*rough;
	float A = 1.0 - 0.5*sigma2/(sigma2 + 0.33);
	float B = 0.45*sigma2/(sigma2 + 0.09);

	// TODO: math here probably isn't right, check cos(theta...)
	return mindot(N, L) * (A + (B*max(0.0, cos(thetai - thetar)*sin(alpha)*tan(beta))));
	//return (1.0/PI) * max(0.0, dot(N, L));
	//return PI*max(0.0, mindot(N, L));
}
