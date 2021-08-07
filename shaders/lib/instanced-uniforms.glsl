#pragma once

uniform mat4 innerTrans;
uniform mat4 outerTrans;

layout (std140) uniform instanceTransforms {
	mat4 transforms[256];
};
