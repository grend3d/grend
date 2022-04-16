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
	std::string curPath = "";

	public:
		enum Usage {
			Reference,
			Copy,
		};

		// TODO: constructors for single empty node, maybe copying other nodes
		sceneComponent(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, this);
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

			load(path, usage);
		}

		virtual ~sceneComponent();
		virtual const char* typeString(void) const { return getTypeName(*this); };

		void load(const std::string& path, enum Usage usage) {
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

			curPath = path;
			node = (usage == Copy)? duplicate(res) : res;
		}

		sceneNode::ptr getNode() {
			return node;
		}

		const std::string& getPath() {
			return curPath;
		}
};

// namespace grend::ecs
}
