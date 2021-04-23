#include <grend/gameEditor.hpp>
#include "player.hpp"

player::~player() {};

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

// TODO: might as well have a resource component
static gameObject::ptr playerModel = nullptr;

player::player(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager)
{

	new boxSpawner(manager, this);
	new movementHandler(manager, this);
	new projectileCollision(manager, this);
	new syncRigidBodyPosition(manager, this);
	rigidBody *body = new rigidBodySphere(manager, this, position, 10.0, 0.5);

	manager->registerComponent(this, "player", this);

	if (!playerModel) {
		// TODO: resource cache
		//playerModel = loadScene(GR_PREFIX "assets/obj/TestGuy/rigged-lowpolyguy.glb");
		SDL_Log("Loading player model...");
		playerModel = loadSceneCompiled("assets/obj/buff-dude-testanim.glb");
		//playerModel = loadSceneCompiled("/home/flux/blender/objects/lowpoly-cc0-guy/low-poly-cc0-guy-fixedimport.gltf");
		playerModel->transform.rotation = glm::quat(glm::vec3(0, -M_PI/2, 0));

		//playerModel->transform.scale = glm::vec3(0.16f);
		playerModel->transform.position = glm::vec3(0, -0.5, 0);
		assert(playerModel != nullptr);
		SDL_Log("got player model");
	}

	node->transform.position = position;
	setNode("model", node, playerModel);
	//setNode("light", node, std::make_shared<gameLightPoint>());
	//setNode("light", node, std::make_shared<gameLightPoint>());
	//auto lit = std::make_shared<gameLightPoint>();
	auto lit = std::make_shared<gameLightSpot>();
	lit->transform.rotation = glm::quat(glm::vec3(0, -M_PI/2, 0));
	lit->transform.position = glm::vec3(0, 0, 1);
	lit->intensity = 200;
	lit->is_static = false;
	lit->casts_shadows = true;
	setNode("light", node, lit);
	character = std::make_shared<animatedCharacter>(playerModel);
	character->setAnimation("idle");

	body->registerCollisionQueue(manager->collisions);
}

player::player(entityManager *manager,
               entity *ent,
               nlohmann::json properties)
	: entity(manager, properties)
{
	manager->registerComponent(this, "player", this);

	if (!playerModel) {
		// TODO: resource cache
		//playerModel = loadScene(GR_PREFIX "assets/obj/TestGuy/rigged-lowpolyguy.glb");
		SDL_Log("Loading player model...");
		playerModel = loadSceneCompiled("assets/obj/buff-dude-testanim.glb");
		playerModel->transform.rotation = glm::quat(glm::vec3(0, -M_PI/2, 0));

		//playerModel->transform.scale = glm::vec3(0.16f);
		playerModel->transform.position = glm::vec3(0, -0.5, 0);
		//bindCookedMeshes();
		assert(playerModel != nullptr);
		SDL_Log("got player model");
	}

	setNode("model", node, playerModel);
	//setNode("light", node, std::make_shared<gameLightPoint>());
	//setNode("light", node, std::make_shared<gameLightPoint>());
	//auto lit = std::make_shared<gameLightPoint>();
	auto lit = std::make_shared<gameLightSpot>();
	lit->transform.rotation = glm::quat(glm::vec3(0, -M_PI/2, 0));
	lit->transform.position = glm::vec3(0, 0, 1);
	lit->intensity = 200;
	lit->is_static = false;
	lit->casts_shadows = true;
	setNode("light", node, lit);
	character = std::make_shared<animatedCharacter>(playerModel);
	character->setAnimation("idle");
}

nlohmann::json player::serialize(entityManager *manager) {
	return entity::serialize(manager);
}

void player::update(entityManager *manager, float delta) {
	rigidBody *body = castEntityComponent<rigidBody*>(manager, this, "rigidBody");
	if (!body) return;

	glm::vec3 vel = body->phys->getVelocity();
	if (glm::length(vel) < 2.0) {
		character->setAnimation("idle");
	} else {
		character->setAnimation("walking");
	}
}
