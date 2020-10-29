#pragma once
#include <lib/utility.glsl>
#include <lib/constants.glsl>

float oren_nayar_diffuse(float rough, vec3 L, vec3 N, vec3 V) {
	// TODO: needs optimization
	float thetai = acos(dot(N, L));
	float thetar = acos(dot(N, V));
	float phii = PI - thetai;
	float phir = PI - thetar;
	float alpha = max(thetai, thetar);
	float beta  = min(thetai, thetar);
	float sigma2 = rough*rough;
	float A = 1.0 - 0.5*sigma2/(sigma2 + 0.33);
	float B = 0.45*sigma2/(sigma2 + 0.09);

	// TODO: math here probably isn't right, check cos(phi...)
	//       guessing that it's the inverse angle of theta?
	return mindot(N, L) * (A + (B*max(0.0, cos(phii - phir)*sin(alpha)*tan(beta))));
	//return mindot(N, L) * (A + B*sin(alpha)*tan(beta));
}
