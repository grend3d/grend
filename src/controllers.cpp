#include <grend/controllers.hpp>

using namespace grendx;

bindFunc grendx::controller::camMovement(camera::ptr cam, float accel) {
	return [=] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_KEYDOWN) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:     cam->setVelocity( cam->direction()*accel); break;
				case SDLK_s:     cam->setVelocity(-cam->direction()*accel); break;
				case SDLK_a:     cam->setVelocity( cam->right()*accel); break;
				case SDLK_d:     cam->setVelocity(-cam->right()*accel); break;
				case SDLK_q:     cam->setVelocity( cam->up()*accel); break;
				case SDLK_e:     cam->setVelocity(-cam->up()*accel); break;
				case SDLK_SPACE: cam->setVelocity( cam->up()*50.f); break;
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
					cam->setVelocity(glm::vec3(0));
					break;

				default: break;
			};
		}

		return MODAL_NO_CHANGE;
	};
}

bindFunc grendx::controller::camMovement2D(camera::ptr cam, float accel) {
	auto curdir = std::make_shared<glm::vec3>(0);
	auto pressed = std::make_shared<std::set<int>>();

	return [=] (SDL_Event& ev, unsigned flags) {
		glm::vec3 dir, right, up;

		dir   = glm::normalize(cam->direction() * glm::vec3(1, 0, 1));
		right = glm::normalize(cam->right() * glm::vec3(1, 0, 1));
		up    = glm::vec3(0, 1, 0);

		if (ev.type == SDL_KEYDOWN && !pressed->count(ev.key.keysym.sym)) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:     *curdir +=  dir; break;
				case SDLK_s:     *curdir += -dir; break;
				case SDLK_a:     *curdir +=  right; break;
				case SDLK_d:     *curdir += -right; break;
				case SDLK_q:     *curdir +=  up; break;
				case SDLK_e:     *curdir += -up; break;
				case SDLK_SPACE: *curdir +=  up; break;
				default: break;
			};

			pressed->insert(ev.key.keysym.sym);

		} else if (ev.type == SDL_KEYUP && pressed->count(ev.key.keysym.sym)) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:     *curdir -=  dir; break;
				case SDLK_s:     *curdir -= -dir; break;
				case SDLK_a:     *curdir -=  right; break;
				case SDLK_d:     *curdir -= -right; break;
				case SDLK_q:     *curdir -=  up; break;
				case SDLK_e:     *curdir -= -up; break;
				case SDLK_SPACE: *curdir -=  up; break;
				default: break;
			};

			pressed->erase(ev.key.keysym.sym);
		}

		cam->setVelocity(*curdir);

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

		cam->setDirection(glm::vec3(
			sin(rel_x*2*M_PI),
			sin(-rel_y*M_PI/2.f),
			-cos(rel_x*2*M_PI)
		));

		return MODAL_NO_CHANGE;
	};
}

bindFunc
grendx::controller::camAngled2D(camera::ptr cam, gameMain *game, float angle) {
	return [=] (SDL_Event& ev, unsigned flags) {
		int x, y;
		int win_x, win_y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
		SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

		x = (x > 0)? x : win_x/2;
		y = (x > 0)? y : win_y/2;

		float center_x = (float)win_x / 2;
		//float center_y = (float)win_y / 2;

		float rel_x = ((float)x - center_x) / center_x;
		//float rel_y = ((float)y - center_y) / center_y;

		// TODO: another function that allows you to pan up and down some amount,
		//       also one that doesn't rotate (fixed orientation)
		cam->setDirection(glm::vec3(
			sin(rel_x*2*M_PI),
			//sin(-rel_y*M_PI/2.f),
			sin(angle),
			-cos(rel_x*2*M_PI)
		));

		return MODAL_NO_CHANGE;
	};
}

bindFunc
grendx::controller::camAngled2DFixed(camera::ptr cam, gameMain *game, float angle) {
	return [=] (SDL_Event& ev, unsigned flags) {
		int x, y;
		int win_x, win_y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
		SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

		x = (x > 0)? x : win_x/2;
		y = (x > 0)? y : win_y/2;

		// TODO: keybinds to rotate, pan up/down
		float center_x = (float)win_x / 2;
		float rel_x = ((float)x - center_x) / center_x;

		cam->setDirection(glm::vec3(0, sin(angle), -cos(angle)));

		return MODAL_NO_CHANGE;
	};
}

bindFunc
grendx::controller::camAngled2DRotatable(camera::ptr cam,
                                         gameMain *game,
                                         float angle,
                                         float minY,
                                         float maxY)
{
	struct moveState {
		glm::ivec2 lastClick;
		glm::vec2 distMoved;

		bool clicked = false;
		float angle;
	};

	// small XXX: need to bind external state somehow, using shared pointer for now
	std::shared_ptr<moveState> state = std::make_shared<moveState>();
	state->angle = angle;

	return [=] (SDL_Event& ev, unsigned flags) {
		if (ev.button.button == SDL_BUTTON_MIDDLE) {
			if (!state->clicked && ev.type == SDL_MOUSEBUTTONDOWN) {
				SDL_GetMouseState(&state->lastClick[0], &state->lastClick[1]);
				//distMoved = {0, 0};
				state->clicked = true;

			} else if (state->clicked && ev.type == SDL_MOUSEBUTTONUP) {
				state->clicked = false;

			} else if (state->clicked && ev.type == SDL_MOUSEMOTION) {
				int win_x, win_y;
				SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

				state->distMoved += glm::vec2 {
					ev.motion.xrel / float(win_x),
					ev.motion.yrel / float(win_y)
				};

				cam->setDirection(glm::vec3 {
					sin(4.f*state->distMoved.x),
					sin(glm::clamp(-state->distMoved.y + angle, minY, maxY)),
					-cos(4.f*state->distMoved.x)
				});
			}
		}

		/*
		int x, y;
		int win_x, win_y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
		SDL_GetWindowSize(game->ctx.window, &win_x, &win_y);

		x = (x > 0)? x : win_x/2;
		y = (x > 0)? y : win_y/2;

		float center_x = (float)win_x / 2;
		//float center_y = (float)win_y / 2;

		float rel_x = ((float)x - center_x) / center_x;
		//float rel_y = ((float)y - center_y) / center_y;

		// TODO: another function that allows you to pan up and down some amount,
		//       also one that doesn't rotate (fixed orientation)
		cam->setDirection(glm::vec3(
			sin(rel_x*2*M_PI),
			//sin(-rel_y*M_PI/2.f),
			sin(angle),
			-cos(rel_x*2*M_PI)
		));
		*/

		return MODAL_NO_CHANGE;
	};
}

bindFunc grendx::controller::camFocus(camera::ptr cam, gameObject::ptr focus) {
	return [=] (SDL_Event& ev, unsigned flags) {
		// TODO: camFocus()
		return MODAL_NO_CHANGE;
	};
}

bindFunc grendx::controller::camScrollZoom(camera::ptr cam, float *zoom, float scale)
{
	return [=] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_MOUSEWHEEL) {
			*zoom -= scale*ev.wheel.y;
		}

		return MODAL_NO_CHANGE;
	};
}

// XXX: don't know where to put this, input handlers for
//      resizing the framebuffer on window resize, etc.
// TODO: maybe dedicated header
bindFunc grendx::resizeInputHandler(gameMain *game, renderPostChain::ptr post) {
	return [=] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_WINDOWEVENT
		    && ev.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			auto width = ev.window.data1;
			auto height = ev.window.data2;

			game->rend->framebuffer->setSize(width, height);

			if (post != nullptr) {
				post->setSize(width, height);
			}
		}

		return MODAL_NO_CHANGE;
	};
}
