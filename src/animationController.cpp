#include <grend/ecs/animationController.hpp>
#include <grend/ecs/sceneComponent.hpp>

#include <grend/utility.hpp>
#include <imgui/imgui.h>

namespace grendx {

void animationController::setAnimation(std::string animation, float weight) {
	if (!animations) return;

	auto it = animations->find(animation);

	if (it != animations->end()) {
		currentAnimation = it->second;

	} else {
		LogWarnFmt("WARNING: setting nonexistant animation '{}'!", animation);
		for (const auto& [name, _] : *animations) {
			LogFmt("Available: {}", name);
		}
	}
}

void applyAnimation(sceneNode::ptr node, animationMap::ptr anim, float time) {
	if (node == nullptr || anim == nullptr) {
		return;
	}

	auto chans = anim->find(node->animChannel);

	if (chans != anim->end()) {
		TRS t = node->transform.getOrig();

		for (auto& ch : chans->second) {
			//SDL_Log("Have animation channel %08x", node->animChannel);
			ch->applyTransform(t, time, anim->endtime);
		}

		node->transform.set(t);
	}

	for (auto link : node->nodes()) {
		applyAnimation(link->getRef(), anim, time);
	}
}

animationController::animationController(regArgs t)
	: component(doRegister(this, t))
{
	manager->registerInterface<updatable>(t.ent, this);

	if (dynamic_cast<sceneNode*>(t.ent)) {
		if (auto *p = dynamic_cast<sceneImport*>(t.ent)) {
			// TODO: copy
			animations = p->animations;
		}
	}

	if (!animations) {
		animations = std::make_shared<animationCollection>();
	}
};

void animationController::bind(std::string name, animationMap::ptr anim) {
	if (!animations) {
		LogWarn("No animation collection set!");
		return;
	}

	animations->insert({name, anim});
}

void animationController::bind(std::string name, std::string path) {
	if (auto obj = loadSceneCompiled(path)) {
		auto& anims = *(*obj)->animations;
		unsigned count = 0;

		for (const auto& [importedName, map] : anims) {
			std::string fullName = name;

			if (count++ > 0) {
				fullName += ":" + importedName;
				//auto fullName = name + ":" + importedName;
				LogWarnFmt("Remapping additional animation for {} to {}", name, fullName);
			}

			this->bind(fullName, map);
			this->paths.push_back({name, path});
		}
	}
}

sceneImport::ptr animationController::getObject(void) {
	return objects;
}

void animationController::update(entityManager *manager, float delta) {
	if (!currentAnimation) return;
	// animTime has gotten into a negative or NaN state
	if (!(animTime == 0 || animTime > 0)) animTime = 0;

	if (currentAnimation->endtime == 0) return;

	entity *ent = manager->getEntity(this);
	// order of priorities for choosing animation targets:
	// - sceneImport node if the node is an import
	// - sceneComponent node if a scene component is attached
	// - sceneNode if the node is any other derived class of sceneNode
	sceneNode *node = dynamic_cast<sceneImport*>(ent);

	if (!node) {
		if (auto *scene = ent->get<sceneComponent>()) {
			node = scene->getNode().getPtr();
		} else {
			node = dynamic_cast<sceneNode*>(ent);
		}
	}

	if (node) {
		animTime = fmod(animTime + delta*animSpeed, currentAnimation->endtime);
		applyAnimation(node, currentAnimation, animTime);
	}
}

void animationController::setSpeed(float speed) {
	animSpeed = speed;
}

nlohmann::json animationController::serializer(component *comp) {
	auto *controller = static_cast<animationController*>(comp);
	nlohmann::json ret = {};

	for (auto& [name, path] : controller->paths) {
		ret[name] = path;
	}

	return ret;
}

void animationController::deserializer(component *comp, nlohmann::json j) {
	auto *controller = static_cast<animationController*>(comp);

	for (auto& [name, path] : j.items()) {
		controller->bind(name, path);
	}
}

// TODO: utility functions
template <typename T>
static T* beginType(component *comp) {
	auto name = demangle(getTypeName<T>());

	ImGui::Text("%s", name.c_str());
	ImGui::Separator();
	ImGui::Indent(16.f);
	ImGui::PushID(0);

	return static_cast<T*>(comp);
}

static void endType() {
	ImGui::PopID();
	ImGui::Unindent(16.f);
}


void animationController::drawEditor(component *comp) {
	auto *ent = beginType<animationController>(comp);
	auto *controller = dynamic_cast<sceneImport*>(comp->manager->getEntity(comp));

	static char name[128];
	static char path[128];

	if (ent->animations) {
		for (auto& [name, anims] : *ent->animations) {
			if (ImGui::Button(name.c_str())) {
				ent->setAnimation(name, 1.0);
			}
		}

	} else {
		ImGui::Text("[No animations?]");
		if (controller && controller->animations) {
			ImGui::Text("Have controller");
		}
	}

	ImGui::Separator();

	for (auto& [name, path] : ent->paths) {
		ImGui::Text("%s", name.c_str());
		ImGui::SameLine();
		ImGui::Text("%s", path.c_str());
	}

	ImGui::InputText("Name", name, sizeof(name));
	ImGui::InputText("Path", path, sizeof(path));

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("DRAG_FILENAME")) {
			const char *fname = (const char*)payload->Data;
			strncpy(path, fname, sizeof(path) - 1);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Add")) {
		ent->bind(name, path);
	}

	endType();
}

// namespace grendx
}
