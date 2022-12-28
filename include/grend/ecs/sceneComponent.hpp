#pragma once

#include <map>
#include <grend/loadScene.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/serializer.hpp>

namespace grendx::ecs {

struct sceneComponentAdded {
	entity *ent;
	sceneNode::ptr node;
};

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

		void emitMessage(entity *ent) {
			sceneComponentAdded msg = {
				.ent = ent,
				.node = this->node,
			};

			LogFmt("Also got here, sending sceneComponentAdded message");
			ent->messages.publish(msg);
		}

		// TODO: constructors for single empty node, maybe copying other nodes
		sceneComponent(regArgs t)
			: component(doRegister(this, t))
		{
		};

		sceneComponent(regArgs t, const std::string& path)
			: sceneComponent(std::move(t), path, Reference) {};

		sceneComponent(regArgs t,
		               const std::string& path,
		               enum Usage usage)
			: component(doRegister(this, t))
		{
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

			if (auto foo = lookup) {
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

			auto resnode = ref_cast<sceneNode>(res);
			node = (usage == Copy)? duplicate(resnode) : resnode;
			emitMessage(manager->getEntity(this));
		}

		sceneNode::ptr getNode() {
			return node;
		}

		const std::string& getPath() {
			return curPath;
		}

		static nlohmann::json serializer(component *comp) {
			sceneComponent *scn = static_cast<sceneComponent*>(comp);

			return {
				{"path", scn->getPath()},
			};
		}

		static void deserializer(component *comp, nlohmann::json j) {
			sceneComponent *scn = static_cast<sceneComponent*>(comp);
			std::string path = tryGet<std::string>(j, "path", "");

			std::cout << "Loading "  << j["path"] << std::endl;

			if (!path.empty()) {
				// TODO: should store usage?????
				scn->load(path, sceneComponent::Reference);
			}
		}
};

// namespace grend::ecs
}
