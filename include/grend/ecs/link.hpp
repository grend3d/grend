#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/ref.hpp>

namespace grendx::ecs {

class baseLink : public component {
	public:
		baseLink(regArgs t)
			: component(doRegister(this, t)),
			  basePtr(nullptr)
			{ }

		baseLink(regArgs t, entity* target)
			: component(doRegister(this, t)),
			  basePtr(target)
			{ }

		~baseLink() {};

		ref<entity> getBaseRef(void) {
			return basePtr;
		}

		static nlohmann::json serializer(component *comp) { return {}; };
		static void deserializer(component *comp, nlohmann::json j) {};

	private:
		ref<entity> basePtr;
};

template <typename T = entity>
class link : public baseLink {
	public:
		link(regArgs t)
			: baseLink(doRegister(this, t)) {}

		link(regArgs t, T* target)
			: baseLink(doRegister(this, t), target) {}

		~link() {};

		ref<T> getRef(void) {
			return ref_cast<T>(getBaseRef());
		}
	
		// TODO: weak ref that avoids taking reference
		T* operator->() { return getRef().operator->(); }
		T const* operator->() const { return getRef().operator->(); }
};

};
