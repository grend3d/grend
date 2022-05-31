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
struct renderFlags {
	Program::ptr mainShader;
	Program::ptr skinnedShader;
	Program::ptr instancedShader;
	Program::ptr billboardShader;
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
	return lhs.mainShader      == rhs.mainShader
	    && lhs.skinnedShader   == rhs.skinnedShader
	    && lhs.instancedShader == rhs.instancedShader
	    && lhs.billboardShader == rhs.billboardShader
#if 0
	    && lhs.features        == rhs.features
	    && lhs.faceOrder       == rhs.faceOrder
	    && lhs.depthFunc       == rhs.depthFunc
	    && lhs.stencilFunc     == rhs.stencilFunc
	    && lhs.stencilFail     == rhs.stencilFail
	    && lhs.depthFail       == rhs.depthFail
	    && lhs.depthPass       == rhs.depthPass
#endif
	    ;
}

// namespace grendx
};

// injecting a hash function for renderflags into std namespace
template <>
struct std::hash<grendx::renderFlags> {
	std::size_t operator()(const grendx::renderFlags& r) const noexcept {
		std::size_t ret = 737;

#if 0
		unsigned k = (r.features    << 14)
		           | (r.faceOrder   << 13)
		           | (r.depthFunc   << 12)
		           | (r.stencilFunc << 9)
		           | (r.stencilFail << 6)
		           | (r.depthFail   << 3)
		           | (r.depthPass   << 0)
		           ;
#endif

		ret = ret*33 + (uintptr_t)r.mainShader.get();
		ret = ret*33 + (uintptr_t)r.skinnedShader.get();
		ret = ret*33 + (uintptr_t)r.instancedShader.get();
		ret = ret*33 + (uintptr_t)r.billboardShader.get();
		//ret = ret*33 + k;

		return ret;
	}
};
