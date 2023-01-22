#pragma once

namespace grendx::ecs {

template <typename T>
class ref {
	public:
		ref(T *target) : ptr(target) {
			// TODO: explicit hard ref on base entity
		};

		ref() : ptr(nullptr) {}

		// TODO: copy, move constructors

		~ref() {
			// TODO: remove reference
		};

		T* operator->() const { return ptr; }
		//T const* operator->() const { return ptr; }
		operator bool() const { return !!ptr; }

		template <typename E>
		ref operator=(const ref<E>& other) {
			ptr = other.getPtr();
			return *this;
		}

		bool operator==(const ref& rhs) { return ptr == rhs.ptr; }
		bool operator==(const T* rhs) { return ptr == rhs; }
		//bool operator==(T* rhs) { return ptr == rhs; }

		T* getPtr() const { return ptr; }

	private:
		T* ptr;
};

template <typename E, typename T>
ref<E> ref_cast(const ref<T>& r) {
	//return ref<E>(r->template get<E>());
	return ref<E>(static_cast<E*>(r.getPtr()));
}

template <typename E, typename T>
ref<E> dynamic_ref_cast(const ref<T>& r) {
	//return ref<E>(r->template get<E>());
	return ref<E>(dynamic_cast<E*>(r.getPtr()));
}

}
