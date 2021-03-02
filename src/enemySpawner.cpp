#include <grend/gameEditor.hpp>

#include "enemySpawner.hpp"
#include "projectile.hpp"
#include "enemy.hpp"
#include "health.hpp"
#include "healthbar.hpp"
#include "team.hpp"

enemySpawner::~enemySpawner() {};

enemySpawner::enemySpawner(entityManager *manager, gameMain *game, glm::vec3 position) 
	: entity(manager)
{
	static gameObject::ptr spawnerModel = nullptr;

	new health(manager, this, 1.f, 1000.f);
	new worldHealthbar(manager, this);
	new projectileCollision(manager, this);
	auto body = new rigidBodySphere(manager, this, position, 0.0, 1.0);

	manager->registerComponent(this, "enemySpawner", this);

	// TODO: resource manager
	if (!spawnerModel) {
		spawnerModel = loadScene("assets/obj/enemy-spawner.glb");
		bindCookedMeshes();
	}

	node->transform.position = position;
	// TODO: this should just be done as part of creating a rigidBody...
	body->registerCollisionQueue(manager->collisions);
	setNode("model", node, spawnerModel);
}

enemySpawner::enemySpawner(entityManager *manager,
                           entity *ent,
                           nlohmann::json properties) 
	: entity(manager, properties)
{
	static gameObject::ptr spawnerModel = nullptr;

	new health(manager, this, 1.f, 1000.f);
	new worldHealthbar(manager, this);
	new projectileCollision(manager, this);
	auto body = new rigidBodySphere(manager, this, node->transform.position,
	                                0.0, 1.0);

	manager->registerComponent(this, "enemySpawner", this);

	// TODO: resource manager
	if (!spawnerModel) {
		spawnerModel = loadScene("assets/obj/enemy-spawner.glb");
		bindCookedMeshes();
	}

	// TODO: this should just be done as part of creating a rigidBody...
	body->registerCollisionQueue(manager->collisions);
	setNode("model", node, spawnerModel);
}

nlohmann::json enemySpawner::serialize(entityManager *manager) {
	return entity::serialize(manager);
}

void enemySpawner::update(entityManager *manager, float delta) {
	float curTime = SDL_GetTicks() / 1000.f;

	if (curTime - lastSpawn > 2.0f) {
		lastSpawn = curTime;

		auto en = new enemy(manager, manager->engine, this->node->transform.position);
		manager->add(en);

		// if this spawner has an associated team, propagate that to
		// spawned enemies
		if (team *tem = castEntityComponent<team*>(manager, this, "team")) {
			new team(manager, en, tem->name);
		}
	}
}
