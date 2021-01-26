#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>
#include <grend/ecs/rigidBody.hpp>
#include "health.hpp"

using namespace grendx;
using namespace grendx::ecs;

class timedLifetime : public component {
	float lifetime;
	float startTime;

	public:
		timedLifetime(entityManager *manager, entity *ent, float _lifetime = 3.f)
			: component(manager, ent),
			  lifetime(_lifetime)
		{
			manager->registerComponent(ent, "timedLifetime", this);
			// TODO: might be a good idea to pass start time in or something,
			//       this could be annoying for automating test stuff
			//
			//       actually, need a timestamp in the render pipeline,
			//       could reuse that for this once that's implemented,
			//       that way everything's synced
			startTime = SDL_GetTicks() / 1000.f;
		}

		virtual void
		checkLifetime(entityManager *manager, entity *ent, float curTime) {
			if (startTime + lifetime < curTime) {
				manager->remove(ent);
			}
		}
};

class lifetimeSystem : public entitySystem {
	public:
		typedef std::shared_ptr<lifetimeSystem> ptr;
		typedef std::weak_ptr<lifetimeSystem>   weakptr;

		virtual void update(entityManager *manager, float delta) {
			float curTime = SDL_GetTicks() / 1000.f;
			auto timers = manager->getComponents("timedLifetime");

			for (auto& it : timers) {
				timedLifetime *lifetime = dynamic_cast<timedLifetime*>(it);
				entity *ent = manager->getEntity(lifetime);

				if (lifetime && ent) {
					lifetime->checkLifetime(manager, ent, curTime);
				}
			}
		}
};
