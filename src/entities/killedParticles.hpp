#pragma once

#include <grend/gameObject.hpp>
#include <grend/ecs/ecs.hpp>

using namespace grendx;
using namespace grendx::ecs;

class killedParticles : public entityEventSystem {
	public:
		killedParticles(std::vector<std::string> _tags)
			: entityEventSystem(_tags) {};

		virtual ~killedParticles();
		virtual void onEvent(entityManager *manager, entity *ent, float delta);
};

class boxParticles : public entity {
	public:
		boxParticles(entityManager *manager, glm::vec3 pos);
		virtual void update(entityManager *manager, float delta);

		gameBillboardParticles::ptr parts;
		float velocities[32];
		float offsets[32];
		float time = 0.0;

		glm::vec3 asdf;
};
