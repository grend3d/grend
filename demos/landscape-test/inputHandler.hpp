#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/modalSDLInput.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

using namespace grendx;
using namespace grendx::ecs;

struct inputEvent {
	enum types {
		move,
		look,
		crouch,
		// left, right, middle click
		primaryAction,
		secondaryAction,
		tertiaryAction,
	} type;

	glm::vec3 data;
	bool active;
};

typedef std::shared_ptr<std::vector<inputEvent>> inputQueue;

class inputHandler : public component {
	public:
		inputHandler(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "inputHandler", this);
		}

		virtual void
		handleInput(entityManager *manager, entity *ent, inputEvent& ev) {
			std::cerr << "handleInput(): got here" << std::endl;
		}
};

class inputHandlerSystem : public entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual void update(entityManager *manager, float delta);

		inputQueue inputs = std::make_shared<std::vector<inputEvent>>();
};

class controllable : public component {
	public: 
		controllable(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "controllable", this);
		}

		void handleInput(inputEvent& ev);
};

// XXX: having basically tag components, waste of space?
class isControlled : public component {
	public: 
		isControlled(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "isControlled", this);
		}
};

class movementHandler : public inputHandler {
	public:
		movementHandler(entityManager *manager, entity *ent)
			: inputHandler(manager, ent)
		{
			manager->registerComponent(ent, "movementHandler", this);
		}

		virtual void
		handleInput(entityManager *manager, entity *ent, inputEvent& ev) {
			if (ev.active && ev.type == inputEvent::types::move) {
				rigidBody *body;
				castEntityComponent(body, manager, ent, "rigidBody");

				if (body && ev.data != lastvel) {
					body->phys->setVelocity(ev.data);
					lastvel = ev.data;
				}
			}
		}

		glm::vec3 lastvel;
};

bindFunc inputMapper(inputQueue q, camera::ptr cam);
