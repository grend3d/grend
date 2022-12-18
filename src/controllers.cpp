#include <grend/gameMain.hpp>
#include <grend/controllers.hpp>
#include <grend/renderContext.hpp>

using namespace grendx;
using namespace grendx::engine;

bindFunc grendx::controller::camMovement(camera::ptr cam, float accel) {
	// TODO: same as movement2D below, need to keep track of velocities per-event
	return [=] (const SDL_Event& ev, unsigned flags) {
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
	auto pressed = std::make_shared<std::map<int, glm::vec3>>();

	return [=] (const SDL_Event& ev, unsigned flags) {
		glm::vec3 dir, right, up;

		dir   = glm::normalize(cam->direction() * glm::vec3(1, 0, 1));
		right = glm::normalize(cam->right() * glm::vec3(1, 0, 1));
		up    = glm::vec3(0, 1, 0);

		if (ev.type == SDL_KEYDOWN && !pressed->count(ev.key.keysym.sym)) {
			glm::vec3 evdir(0);

			switch (ev.key.keysym.sym) {
				case SDLK_w:     evdir =  dir; break;
				case SDLK_s:     evdir = -dir; break;
				case SDLK_a:     evdir =  right; break;
				case SDLK_d:     evdir = -right; break;
				case SDLK_q:     evdir =  up; break;
				case SDLK_e:     evdir = -up; break;
				case SDLK_SPACE: evdir =  up; break;
				default: break;
			};

			pressed->insert({ev.key.keysym.sym, evdir});
			*curdir += evdir;

		} else if (ev.type == SDL_KEYUP && pressed->count(ev.key.keysym.sym)) {
			auto it = pressed->find(ev.key.keysym.sym);
			auto& [_, evdir] = *it;

			pressed->erase(ev.key.keysym.sym);
			*curdir -= evdir;
		}

		cam->setVelocity(*curdir);

		return MODAL_NO_CHANGE;
	};
}

bindFunc grendx::controller::camFPS(camera::ptr cam) {
	return [=] (const SDL_Event& ev, unsigned flags) {
		auto ctx = Resolve<SDLContext>();

		int x, y;
		int win_x, win_y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
		SDL_GetWindowSize(ctx->window, &win_x, &win_y);

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

// TODO: probably not going to use this anymore now that camAngled2DRotatable()
//       is here, could replace this with camAngled2DRotatable()
bindFunc
grendx::controller::camAngled2D(camera::ptr cam, float angle) {
	return [=] (const SDL_Event& ev, unsigned flags) {
		auto ctx = Resolve<SDLContext>();

		int x, y;
		int win_x, win_y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
		SDL_GetWindowSize(ctx->window, &win_x, &win_y);

		x = (x > 0)? x : win_x/2;
		y = (x > 0)? y : win_y/2;

		float center_x = (float)win_x / 2;
		float rel_x = ((float)x - center_x) / center_x;

		cam->setDirection(glm::vec3(
			sin(rel_x*2*M_PI),
			sin(angle),
			-cos(rel_x*2*M_PI)
		));

		return MODAL_NO_CHANGE;
	};
}

// TODO: pitch and yaw ("angle" doesn't mean anything)
bindFunc
grendx::controller::camAngled2DFixed(camera::ptr cam, float angle) {
	return [=] (const SDL_Event& ev, unsigned flags) {
		auto ctx = Resolve<SDLContext>();

		int x, y;
		int win_x, win_y;
		Uint32 buttons = SDL_GetMouseState(&x, &y); (void)buttons;
		SDL_GetWindowSize(ctx->window, &win_x, &win_y);

		x = (x > 0)? x : win_x/2;
		y = (x > 0)? y : win_y/2;

		cam->setDirection(glm::vec3(0, sin(angle), -cos(angle)));

		return MODAL_NO_CHANGE;
	};
}

// TODO: pitch and yaw ("angle" doesn't mean anything)
bindFunc
grendx::controller::camAngled2DRotatable(camera::ptr cam,
                                         float angle,
                                         float minY,
                                         float maxY)
{
	struct moveState {
		glm::ivec2 lastClick;
		glm::vec2 distMoved;

		bool clicked = false;
		bool initialized = false;
		float angle;
	};

	// small XXX: need to bind external state somehow, using shared pointer for now
	std::shared_ptr<moveState> state = std::make_shared<moveState>();
	state->angle = angle;

	return [=] (const SDL_Event& ev, unsigned flags) {
		auto ctx = Resolve<SDLContext>();
		int x, y; (void)x; (void)y;

		bool isMouse = ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP || ev.type == SDL_MOUSEMOTION;
		bool isController = ev.type == SDL_CONTROLLERBUTTONDOWN || ev.type == SDL_CONTROLLERBUTTONUP;

		if (!state->initialized) {
			cam->setDirection(glm::vec3 {
				sin(4.f*state->distMoved.x),
				sin(glm::clamp(-state->distMoved.y + angle, minY, maxY)),
				-cos(4.f*state->distMoved.x)
			});

			state->initialized = true;
		}

		else if (isMouse) {
			bool isMiddle = ev.button.button == SDL_BUTTON_MIDDLE;

			if (!state->clicked && ev.type == SDL_MOUSEBUTTONDOWN && isMiddle) {
				SDL_GetMouseState(&state->lastClick[0], &state->lastClick[1]);
				//distMoved = {0, 0};
				state->clicked = true;

			} else if (state->clicked && ev.type == SDL_MOUSEBUTTONUP && isMiddle) {
				state->clicked = false;

			} else if (state->clicked && ev.type == SDL_MOUSEMOTION) {
				int win_x, win_y;
				SDL_GetWindowSize(ctx->window, &win_x, &win_y);

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

		else if (isController) {
			float n = 0;

			switch (ev.cbutton.button) {
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
					n = -0.07;
					break;
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
					n = 0.06;
					break;
			}

			state->distMoved.x += n;
			cam->setDirection(glm::vec3 {
				sin(4.f*state->distMoved.x),
				sin(glm::clamp(-state->distMoved.y + angle, minY, maxY)),
				-cos(4.f*state->distMoved.x)
			});
		}

		return MODAL_NO_CHANGE;
	};
}

bindFunc grendx::controller::camFocus(camera::ptr cam, sceneNode::ptr focus) {
	return [=] (const SDL_Event& ev, unsigned flags) {
		// TODO: camFocus()
		return MODAL_NO_CHANGE;
	};
}

bindFunc grendx::controller::camScrollZoom(camera::ptr cam, float *zoom, float scale)
{
	return [=] (const SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_MOUSEWHEEL) {
			*zoom -= scale*ev.wheel.y;
		}

		if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
			if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
				*zoom -= scale;
			}

			if (ev.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
				*zoom += scale;
			}
		}

		return MODAL_NO_CHANGE;
	};
}

// XXX: don't know where to put this, input handlers for
//      resizing the framebuffer on window resize, etc.
// TODO: dedicated file for generic close, minimize, copy/paste, drag and drop, etc
//       handlers... windowControllers.cpp?
bindFunc grendx::resizeInputHandler(renderPostChain::ptr post) {
	return [=] (const SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_WINDOWEVENT
		    && ev.window.event == SDL_WINDOWEVENT_RESIZED)
		{
			auto rend = Resolve<renderContext>();

			auto width = ev.window.data1;
			auto height = ev.window.data2;

			float scaleX = rend->settings.scaleX;
			float scaleY = rend->settings.scaleY;

			rend->framebuffer->setSize(width*scaleX, height*scaleY);

			if (post != nullptr) {
				post->setSize(width, height);
			}
		}

		return MODAL_NO_CHANGE;
	};
}
