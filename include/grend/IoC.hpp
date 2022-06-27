#pragma once

#include <unordered_map>
#include <ranges>
#include <vector>

namespace grendx::IoC {

template <typename T>
const char *getTypeName() {
	return typeid(T).name();
}

class Service {
	public:
		virtual ~Service();
};

class Container {
	using Bindmap = std::unordered_map<const char *, Service*>;
	std::vector<Bindmap> bindStack;

	public:
		Container() {
			pushScope();
		}

		size_t pushScope(void) {
			bindStack.push_back({});
			return bindStack.size() - 1;
		}

		void popScope(void) {
			if (bindStack.empty())
				return;

			for (auto& [handle, service] : bindStack.back()) {
				delete service;
			}

			bindStack.pop_back();
		}

		void restore(size_t index) {
			while (bindStack.size() > index) {
				popScope();
			}
		}

		template <typename Target, typename Type, typename... Args>
		void bind(Args&&... args) noexcept {
			static_assert(std::is_base_of<Target, Type>::value,
			              "Given service type must be derived from the target binding type");
			static_assert(std::is_base_of<Service, Target>::value,
			              "Target service type must be derived from Service");

			if (bindStack.empty())
				pushScope();

			const char *handle = getTypeName<Target>();
			bindStack.back()[handle] = new Type(args...);
		}

		template <typename Target>
		void bind(Target *ptr) {
			static_assert(std::is_base_of<Service, Target>::value,
			              "Target service type must be derived from Service");

			if (bindStack.empty())
				pushScope();

			const char *handle = getTypeName<Target>();
			auto& bindings = bindStack.back();

			if (bindings.contains(handle)) {
				delete bindings[handle];
			}

			bindings[handle] = ptr;
		}

		template <typename Target>
		Target* resolve() {
			auto ret = tryResolve<Target>();

			if (!ret) {
				throw std::logic_error(std::string("Could not resolve required service ")
				                       + std::string(getTypeName<Target>()));
			}

			return ret;
		}

		template <typename Target>
		Target* tryResolve() noexcept {
			const char *handle = getTypeName<Target>();

			for (auto& bindings : bindStack | std::views::reverse) {
				if (bindings.contains(handle)) {
					return static_cast<Target*>(bindings[handle]);
				}
			}

			return nullptr;
		}
};

};
