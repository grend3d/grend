#include <grend/controllers.hpp>

using namespace grendx;

bindFunc grendx::controller::camMovement(camera::ptr cam, float accel) {
	return [=] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_KEYDOWN) {
			switch (ev.key.keysym.sym) {
				case SDLK_w: cam->velocity =  cam->direction*accel; break;
				case SDLK_s: cam->velocity = -cam->direction*accel; break;
				case SDLK_a: cam->velocity =  cam->right*accel; break;
				case SDLK_d: cam->velocity = -cam->right*accel; break;
				case SDLK_q: cam->velocity =  cam->up*accel; break;
				case SDLK_e: cam->velocity = -cam->up*accel; break;
				case SDLK_SPACE: cam->velocity = cam->up*50.f; break;
				default: break;
			};

		} else if (ev.type == SDL_KEYUP) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:
				case SDLK_s:
				case SDLK_a:
				case SDLK_d:
				case SDLK_q:
				case SDLK_e:
				case SDLK_SPACE:
					cam->velocity = glm::vec3(0);
					break;

				default: break;
			};
		}

		return MODAL_NO_CHANGE;
	};
}

bindFunc grendx::controller::camFPS(camera::ptr cam, gameMain *game) {
	return [=] (SDL_Event& ev, unsigned flags) {
		int x, y;
		int win_x, win_y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
		SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

		x = (x > 0)? x : win_x/2;
		y = (x > 0)? y : win_y/2;

		float center_x = (float)win_x / 2;
		float center_y = (float)win_y / 2;

		float rel_x = ((float)x - center_x) / center_x;
		float rel_y = ((float)y - center_y) / center_y;

		cam->set_direction(glm::vec3(
			sin(rel_x*2*M_PI),
			sin(-rel_y*M_PI/2.f),
			-cos(rel_x*2*M_PI)
		));

		return MODAL_NO_CHANGE;
	};
}

bindFunc grendx::controller::camFocus(camera::ptr cam, gameObject::ptr focus) {
	return [=] (SDL_Event& ev, unsigned flags) {
		// TODO: camFocus()
		return MODAL_NO_CHANGE;
	};
}
