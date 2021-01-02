#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>

#include "projectile.hpp"
#include "enemy.hpp"
#include "health.hpp"
#include "healthbar.hpp"

enemy::enemy(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager),
	// TODO: shouldn't keep strong reference to body
	  body(new rigidBodySphere(manager, this, position, 1.0, 1.0))
{
	static gameObject::ptr enemyModel = nullptr;

	new health(manager, this);
	new worldHealthbar(manager, this);
	new projectileCollision(manager, this);
	new syncRigidBodyXZVelocity(manager, this);

	manager->registerComponent(this, "enemy", this);

	// TODO:
	if (!enemyModel) {
#define LOCAL_BUILD 1
#if LOCAL_BUILD
		enemyModel = loadScene("test-assets/obj/test-enemy.glb");
		enemyModel->transform.scale = glm::vec3(0.2);
#else
		gameModel::ptr mod = generate_cuboid(1.0, 2.0, 0.5);
		enemyModel = std::make_shared<gameObject>();
		compileModel("emeny", mod);
		setNode("model", enemyModel, mod);
#endif
	}

	node->transform.position = position;
	setNode("model", node, enemyModel);
	body->registerCollisionQueue(manager->collisions);
	body->phys->setAngularFactor(0.0);
}

void enemy::update(entityManager *manager, float delta) {
	glm::vec3 playerPos;

	entity *playerEnt =
		findNearest(manager, node->transform.position, {"player"});

	if (playerEnt) {
		playerPos = playerEnt->getNode()->transform.position;
	}

	// TODO: should this be a component, a generic chase implementation?
	glm::vec3 diff = playerPos - node->transform.position;
	glm::vec3 vel =  glm::normalize(glm::vec3(diff.x, 0, diff.z));
	body->phys->setAcceleration(10.f*vel);

	/*
	body->syncPhysics(this);
	*/

	collisions->clear();
}
