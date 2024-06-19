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

		void load(const std::string& path, enum Usage usage) {
			if (auto temp = loadSceneCompiled(path)) {
				auto ecs = grendx::engine::Resolve<entityManager>();
				auto resnode = ref_cast<sceneNode>(*temp);
				entity *ent = ecs->getEntity(this);

				emitMessage(manager->getEntity(this));
				this->node    = resnode;
				this->curPath = path;

				// TODO: ideally there should be a better way to check if the entity is a node
				if (sceneNode *pnode = dynamic_cast<sceneNode*>(ent)) {
					setNode(node->name, pnode, node);
				}

			} else {
				printError(temp);
				return;
			}
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

		static void drawEditor(component *comp);
		static const size_t BufSize = 256;
		using Buf = std::array<char, BufSize>;
		Buf *editBuffer = nullptr;
};

// namespace grend::ecs
}
