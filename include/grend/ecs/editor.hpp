#pragma once

#include <grend/sceneNode.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/IoC.hpp>

#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <memory>
#include <initializer_list>

#include <nlohmann/json.hpp>
#include <grend/utility.hpp>

namespace grendx::ecs {

// TODO: is there a way to have this be templated so that the draw function doesn't have to cast?
using EditorDrawer = std::function<void(component *comp)>;

template <typename T>
concept DrawableComponent
	= requires(T a) {
		(EditorDrawer)T::drawEditor;
	};

class editor : public IoC::Service {
	public:
		template <class T>
		void add(const EditorDrawer& drawer) {
			const char *name = getTypeName<T>();
			const std::string& demangled = demangle(name);

			std::cerr << "Registering widget drawer for component "
				<< demangled << " (" << name << "@" << (void*)name << ")"
				<< std::endl;

			drawers[demangled] = drawer;
		}

		template <DrawableComponent T>
		void add() {
			add<T>(T::drawEditor);
		}

		nlohmann::json serialize(entityManager *manager, entity *ent);

		bool has(const std::string& name) {
			auto f = drawers.find(name);
			return f != drawers.end();
		}

		void draw(entity *ent) {
			if (!ent) {
				return;
			}

			auto* manager = ent->manager;

			auto drawComponent = [&] (component *comp) {
				for (auto& subtype : manager->componentTypes[comp]) {
					const std::string& demangled = demangle(subtype);

					if (has(demangled)) {
						drawers[demangled](comp);
					}
				}
			};

			drawComponent(ent);

			for (component *comp : ent->getAll<component>()) {
				drawComponent(comp);
			}
		}

		std::map<std::string, EditorDrawer> drawers;
};

// namespace grendx::ecs
}
