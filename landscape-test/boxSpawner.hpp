#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

#include "inputHandler.hpp"
#include "projectile.hpp"

using namespace grendx;
using namespace grendx::ecs;

class boxBullet : public projectile {
	public:
		boxBullet(entityManager *manager, gameMain *game, glm::vec3 position);
};

class boxSpawner : public inputHandler {
	public:
		boxSpawner(entityManager *manager, entity *ent)
			: inputHandler(manager, ent)
		{
			manager->registerComponent(ent, "boxSpawner", this);
		}

		virtual void
		handleInput(entityManager *manager, entity *ent, inputEvent& ev) {
			if (ev.active && ev.type == inputEvent::types::primaryAction) {
				std::cerr << "boxSpawner::handleInput(): got here" << std::endl;

				auto box = new boxBullet(manager, manager->engine, ent->node->transform.position + 3.f*ev.data);

				rigidBody *body =
					castEntityComponent<rigidBody*>(manager, box, "rigidBody");

				if (body) {
					body->phys->setVelocity(40.f * ev.data);
					manager->add(box);
				}
			}
		}
};
