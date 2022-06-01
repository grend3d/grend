#pragma once

#include <grend/glManager.hpp>
#include <stdint.h>
#include <functional>

namespace grendx {

struct renderOptions {
	enum Features {
		CullFaces   = 1 << 0,
		StencilTest = 1 << 1,
		DepthTest   = 1 << 2,
		DepthMask   = 1 << 3,
		// XXX: TODO: remove this
		Shadowmap   = 1 << 4,
	};

	enum FaceOrder {
		CW,
		CCW
	};

	enum StencilOp {
		Zero,
		Keep,
		Replace,
		Increment,
		IncrementWrap,
		Decrement,
		DecrementWrap,
		Invert,
	};

	enum ValueFunc {
		Never,
		Less,
		LessEqual,
		Equal,
		Greater,
		GreaterEqual,
		NotEqual,
		Always,
	};

	uint32_t features = Features::CullFaces
	                  | Features::DepthTest
	                  | Features::DepthMask
	                  ;

	enum FaceOrder faceOrder   = FaceOrder::CCW;
	enum ValueFunc depthFunc   = ValueFunc::Less;
	enum ValueFunc stencilFunc = ValueFunc::Always;
	enum StencilOp stencilFail = StencilOp::Keep;
	enum StencilOp depthFail   = StencilOp::Keep;
	enum StencilOp depthPass   = StencilOp::Replace;

	// TODO: move to OpenGL backend once that's split off
	static inline GLenum valueToGL(ValueFunc v) {
		switch (v) {
			case Never:        return GL_NEVER;
			case Less:         return GL_LESS;
			case LessEqual:    return GL_LEQUAL;
			case Equal:        return GL_EQUAL;
			case Greater:      return GL_GREATER;
			case GreaterEqual: return GL_GEQUAL;
			case NotEqual:     return GL_NOTEQUAL;
			case Always:       return GL_ALWAYS;

			// XXX:
			default: return GL_INVALID_ENUM;
		}
	}

	static inline GLenum stencilOpToGL(StencilOp v) {
		switch (v) {
			case Zero:          return GL_ZERO;
			case Keep:          return GL_KEEP;
			case Replace:       return GL_REPLACE;
			case Increment:     return GL_INCR;
			case IncrementWrap: return GL_INCR_WRAP;
			case Decrement:     return GL_DECR;
			case DecrementWrap: return GL_DECR_WRAP;
			case Invert:        return GL_INVERT;

			// XXX:
			default: return GL_INVALID_ENUM;
		}
	}
};

// TODO: rename renderShaders
// hmm, shader permutations... an ominous development :O
struct renderFlags {
	enum shaderTypes {
		Main,
		Skinned,
		Instanced,
		Billboard,
		MaxShaders
	};

	enum variantTypes {
		Opaque,
		Masked,
		DitheredBlend,
		MaxVariants
	};

	struct shaderVariant {
		Program::ptr shaders[MaxShaders];

		bool operator==(const shaderVariant& other) const {
			for (unsigned i = 0; i < MaxShaders; i++) {
				if (shaders[i] != other.shaders[i]) {
					return false;
				}
			}

			return true;
		}
	};

	renderFlags() {
		for (auto& var : variants) {
			for (auto& shader : var.shaders) {
				shader.reset();
			}
		}
	}

	shaderVariant variants[MaxVariants];
};

static inline bool hasFlag(const uint32_t& value, uint32_t flag) {
	return value & flag;
}

static inline void setFlag(uint32_t& value, uint32_t flag) {
	value |= flag;
}

static inline void unsetFlag(uint32_t& value, uint32_t flag) {
	value &= ~flag;
}

static inline
bool operator==(const renderFlags& lhs, const renderFlags& rhs) noexcept {
	for (unsigned v = 0; v < renderFlags::MaxVariants; v++) {
		if (lhs.variants[v] != rhs.variants[v]) {
			return false;
		}
	}

	return true;
}

// namespace grendx
};

// injecting a hash function for renderflags into std namespace
template <>
struct std::hash<grendx::renderFlags> {
	std::size_t operator()(const grendx::renderFlags& r) const noexcept {
		std::size_t ret = 737;

		for (auto& var : r.variants) {
			for (auto& shader : var.shaders) {
				ret = ret*33 + (uintptr_t)shader.get();
			}
		}

		return ret;
	}
};
