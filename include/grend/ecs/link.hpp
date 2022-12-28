#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/ref.hpp>

namespace grendx::ecs {

template <typename T = entity>
class link : public component {
	public:
		link(regArgs t, T* target)
			: component(doRegister(this, t)),
			  ptr(target)
			{ }
		~link() {};

		ref<T> getRef(void) {
			return ptr;
		}
	
		T* operator->() { return ptr.operator->(); }
		T const* operator->() const { return ptr.operator->(); }

	private:
		ref<T> ptr;
		//T* ptr;
};

};
