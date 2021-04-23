#include "inputHandler.hpp"

void inputHandlerSystem::update(entityManager *manager, float delta) {
	auto handlers  = manager->getComponents("inputHandler");
	// TODO: maybe have seperate system for pollers
	auto pollers   = manager->getComponents("inputPoller");

	for (auto& ev : *inputs) {
		for (auto& it : handlers) {
			inputHandler *handler = dynamic_cast<inputHandler*>(it);
			entity *ent = manager->getEntity(handler);

			if (handler && ent) {
				handler->handleInput(manager, ent, ev);
			}
		}
	}

	for (auto& it : pollers) {
		inputPoller *poller = dynamic_cast<inputPoller*>(it);
		entity *ent = manager->getEntity(poller);

		if (poller && ent) {
			poller->update(manager, ent);
		}
	}

	inputs->clear();
}

void inputHandlerSystem::handleEvent(entityManager *manager, SDL_Event& ev) {
	auto rawEvents = manager->getComponents("rawEventHandler");

	for (auto& it : rawEvents) {
		rawEventHandler *handler = dynamic_cast<rawEventHandler*>(it);
		entity *ent = manager->getEntity(handler);

		if (handler && ent) {
			handler->handleEvent(manager, ent, ev);
		}
	}
}

void mouseRotationPoller::update(entityManager *manager, entity *ent) {
	int x, y, win_x, win_y;

	// TODO: this could be passed as a parameter to avoid calling SDL_*
	//       functions redundantly, don't think it'll be a problem though
	SDL_GetMouseState(&x, &y);
	SDL_GetWindowSize(manager->engine->ctx.window, &win_x, &win_y);

	glm::vec2 pos(x * 1.f/win_x, y * 1.f/win_y);
	glm::vec2 center(0.5);
	glm::vec2 diff = pos - center;
	glm::quat rot(glm::vec3(0, atan2(diff.x, diff.y), 0));

	ent->node->transform.rotation = rot;
}

void touchMovementHandler::handleEvent(entityManager *manager,
                                       entity *ent,
                                       SDL_Event& ev)
{
	if (ev.type == SDL_FINGERMOTION || ev.type == SDL_FINGERDOWN) {
		float x = ev.tfinger.x;
		float y = ev.tfinger.y;
		int wx = manager->engine->rend->screen_x;
		int wy = manager->engine->rend->screen_y;

		glm::vec2 touch(wx * x, wy * y);
		glm::vec2 diff = center - touch;
		float     dist = glm::length(diff) / 150.f;

		if (dist < 1.0) {
			glm::vec3 dir = (cam->direction()*diff.y + cam->right()*diff.x) / 15.f;
			SDL_Log("MOVE: %g (%g,%g,%g)", dist, dir.x, dir.y, dir.z);
			touchpos = -diff;
			inputs->push_back({
				.type = inputEvent::types::move,
				.data = dir,
				.active = true,
			});
		}

		SDL_Log("MOVE: got finger touch, %g (%g, %g)", dist, touch.x, touch.y);
	}
}

void touchRotationHandler::handleEvent(entityManager *manager,
                                       entity *ent,
                                       SDL_Event& ev)
{
	if (ev.type == SDL_FINGERMOTION || ev.type == SDL_FINGERDOWN) {
		static uint32_t last_action = 0;

		float x = ev.tfinger.x;
		float y = ev.tfinger.y;
		int wx = manager->engine->rend->screen_x;
		int wy = manager->engine->rend->screen_y;

		glm::vec2 touch(wx * x, wy * y);
		glm::vec2 diff = center - touch;
		float     dist = glm::length(diff) / 150.f;
		uint32_t  ticks = SDL_GetTicks();

		if (dist < 1.0) {
			glm::vec3 dir = (cam->direction()*diff.y + cam->right()*diff.x) / 15.f;
			SDL_Log("ACTION: %g (%g,%g,%g)", dist, dir.x, dir.y, dir.z);
			touchpos = -diff;

			glm::quat rot(glm::vec3(0, atan2(touchpos.x, touchpos.y), 0));
			ent->node->transform.rotation = rot;

			if (ticks - last_action > 500) {
				last_action = ticks;
				inputs->push_back({
					.type = inputEvent::types::primaryAction,
					.data = glm::normalize(dir),
					.active = true,
				});
			}
		}

		SDL_Log("ACTION: got finger touch, %g (%g, %g)", dist, touch.x, touch.y);
	}
}

bindFunc inputMapper(inputQueue q, camera::ptr cam) {
	return [=] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:
				case SDLK_s:
				case SDLK_a:
				case SDLK_d:
				case SDLK_q:
				case SDLK_e:
				case SDLK_SPACE:
					// XXX: just sync movement with camera, will want
					//      to change this
					q->push_back({
						.type = inputEvent::types::move,
						.data = cam->velocity(),
						.active = ev.type == SDL_KEYDOWN,
					});
					break;
			}
		}

		if (ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP) {
			// down = active (clicked), up = inactive
			bool active = ev.type == SDL_MOUSEBUTTONDOWN;

			switch (ev.button.button) {
				case SDL_BUTTON_LEFT:
					q->push_back({
						.type = inputEvent::types::primaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_BUTTON_RIGHT:
					q->push_back({
						.type = inputEvent::types::secondaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_BUTTON_MIDDLE:
					q->push_back({
						.type = inputEvent::types::tertiaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;
			}
		}

		return MODAL_NO_CHANGE;
	};
}
