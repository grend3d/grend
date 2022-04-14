#pragma once

#include <map>
#include <grend/loadScene.hpp>
#include <grend/ecs/ecs.hpp>

namespace grendx::ecs {

// TODO: proper cache for this, need to have a more universal cache
//       (something to also work with the model loading code)
inline std::map<std::string, sceneImport::weakptr> sceneCache;

class sceneComponent : public component {
	sceneNode::ptr node = nullptr;

	public:
		enum Usage {
			Reference,
			Copy,
		};

		sceneComponent(entityManager *manager,
		               entity *ent,
		               const std::string& path)
			: sceneComponent(manager, ent, path, Reference) {};

		sceneComponent(entityManager *manager,
		               entity *ent,
		               const std::string& path,
		               enum Usage usage)
			: component(manager, ent)
		{
			manager->registerComponent(ent, this);

			auto it = sceneCache.find(path);
			sceneImport::weakptr lookup;
			sceneImport::ptr res;

			if (it != sceneCache.end()) {
				lookup = it->second;
			}

			if (auto foo = lookup.lock()) {
				res = foo;

			} else {
				if (auto temp = loadSceneCompiled(path)) {
					sceneCache[path] = *temp;
					res = *temp;

				} else {
					printError(temp);
					return;
				}
			}

			node = (usage == Copy)? duplicate(res) : res;
		}

		sceneNode::ptr getNode() {
			return node;
		}
};

// namespace grend::ecs
}
