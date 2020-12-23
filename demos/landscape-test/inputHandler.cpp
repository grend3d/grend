#include "inputHandler.hpp"

void inputHandlerSystem::update(entityManager *manager, float delta) {
	auto handlers = manager->getComponents("inputHandler");

	for (auto& ev : *inputs) {
		for (auto& it : handlers) {
			inputHandler *handler = dynamic_cast<inputHandler*>(it);
			entity *ent = manager->getEntity(handler);

			if (handler && ent) {
				handler->handleInput(manager, ent, ev);
			}
		}
	}

	inputs->clear();
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
