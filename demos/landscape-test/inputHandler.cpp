#include "inputHandler.hpp"

void inputHandlerSystem::update(entityManager *manager, float delta) {
	auto handlers = manager->getComponents("inputHandler");
	// TODO: maybe have seperate system for pollers
	auto pollers  = manager->getComponents("inputPoller");

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
