#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <components/health.hpp>

using namespace grendx;
using namespace grendx::ecs;

class pickup : public entity {
	public:
		pickup(entityManager *manager)
			: entity(manager)
		{
			manager->registerComponent(this, "pickup", this);
		}

		virtual void apply(entityManager *manager, entity *ent) const = 0;
		virtual void update(entityManager *manager, float delta) {};
};

class healthPickup : public pickup {
	float heals = 30.f;

	public:
		healthPickup(entityManager *manager, glm::vec3 position)
			: pickup(manager)
		{
			gameLightPoint::ptr lit = std::make_shared<gameLightPoint>();

			manager->registerComponent(this, "healthPickup", this);
			// 0 mass, static position
			new rigidBodySphere(manager, this, position, 0.5, 0.5);
			new syncRigidBodyTransform(manager, this);

			static gameModel::ptr model = nullptr;
			// XXX: really need resource manager
			if (model == nullptr) {
				model = load_object(GR_PREFIX "assets/obj/smoothsphere.obj");
				compileModel("healthmodel", model);
				bindCookedMeshes();
			}

			lit->diffuse = glm::vec4(1, 0, 0, 1); // red
			model->transform.scale = glm::vec3(0.5);

			setNode("model", node, model);
			setNode("light", node, lit);

			node->transform.position = position;
		}

		virtual void apply(entityManager *manager, entity *ent) const {
			//health *enthealth = manager->getEnt
			auto comps = manager->getEntityComponents(ent);
			auto range = comps.equal_range("health");

			for (auto it = range.first; it != range.second; it++) {
				auto& [key, comp] = *it;
				health *entHealth = dynamic_cast<health*>(comp);

				if (entHealth) {
					entHealth->heal(heals);
				}
			}
		}
};

class healthPickupCollision : public collisionHandler {
	float damage;
	float lastCollision = 0;

	public:
		healthPickupCollision(entityManager *manager, entity *ent, float _damage = 2.5f)
			: collisionHandler(manager, ent, {"healthPickup"})
		{
			damage = _damage;
			manager->registerComponent(ent, "healthPickupCollision", this);
		}

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col)
		{
			std::cerr << "health pickup collision!" << std::endl;
			healthPickup *pickup = dynamic_cast<healthPickup*>(other);

			if (pickup) {
				pickup->apply(manager, ent);
				manager->remove(pickup);
			}
		};
};
