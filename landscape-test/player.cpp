#include <grend/gameEditor.hpp>
#include "player.hpp"

// TODO: only returns one collection, object tree could have any number
animationCollection::ptr findAnimations(gameObject::ptr obj) {
	if (!obj) {
		return nullptr;
	}

	for (auto& chan : obj->animations) {
		return chan->group->collection;
	}

	for (auto& [name, ptr] : obj->nodes) {
		auto ret = findAnimations(ptr);

		if (ret) {
			return ret;
		}
	}

	return nullptr;
}

void animatedCharacter::setAnimation(std::string animation, float weight) {
	if (animations) {
		for (auto& [name, ptr] : animations->groups) {
			if (auto group = ptr.lock()) {
				group->weight = (name == animation)? weight : 0.0;
			}
		}
	}
}

animatedCharacter::animatedCharacter(gameObject::ptr objs) {
	if ((animations = findAnimations(objs)) == nullptr) {
		// TODO: proper error
		throw "asdf";
	}

	objects = objs;
}

gameObject::ptr animatedCharacter::getObject(void) {
	return objects;
}

player::player(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager),
	  body(new rigidBodySphere(manager, this, position, 1.0, 1.0))
{
	static gameObject::ptr playerModel = nullptr;

	new boxSpawner(manager, this);
	new movementHandler(manager, this);
	new projectileCollision(manager, this);

	manager->registerComponent(this, "player", this);

	if (!playerModel) {
		// TODO: resource cache
		playerModel = loadScene(GR_PREFIX "assets/obj/TestGuy/rigged-lowpolyguy.glb");
		playerModel->transform.scale = glm::vec3(0.1f);
		playerModel->transform.position = glm::vec3(0, -1, 0);
		bindCookedMeshes();
	}

	node->transform.position = position;
	setNode("model", node, playerModel);
	character = std::make_shared<animatedCharacter>(playerModel);
	character->setAnimation("idle");

	body->registerCollisionQueue(manager->collisions);
}

void player::update(entityManager *manager, float delta) {
	TRS physTransform = body->phys->getTransform();
	node->transform.position = physTransform.position;
	glm::vec3 vel = body->phys->getVelocity();
	node->transform.rotation =
		glm::quat(glm::vec3(0, atan2(vel.x, vel.z), 0));

	if (glm::length(vel) < 2.0) {
		character->setAnimation("idle");
	} else {
		character->setAnimation("walking");
	}
}
