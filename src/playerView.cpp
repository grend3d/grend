#include <grend/playerView.hpp>
#include <grend/gameView.hpp>
#include <glm/gtx/rotate_vector.hpp>

using namespace grendx;

static renderPostStage<rOutput>::ptr testpost = nullptr;
static gameObject::ptr testweapon = nullptr;

playerView::playerView(gameMain *game) : gameView() {
	static const float speed = 15.f;

	cameraPhysID = game->phys->add_sphere(cameraObj, glm::vec3(0, 10, 0), 1.0, 1.0);
	setNode("player camera", game->state->physObjects, cameraObj);

	// movement controls
	input.bind(modes::Move,
		[&] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_w: cam->velocity =  cam->direction*speed; break;
					case SDLK_s: cam->velocity = -cam->direction*speed; break;
					case SDLK_a: cam->velocity =  cam->right*speed; break;
					case SDLK_d: cam->velocity = -cam->right*speed; break;
					case SDLK_q: cam->velocity =  cam->up*speed; break;
					case SDLK_e: cam->velocity = -cam->up*speed; break;
					case SDLK_SPACE: cam->velocity = cam->up*50.f; break; default: break;
				};
			}
			return MODAL_NO_CHANGE;
		});

	input.bind(modes::Move,
		[&] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYUP) {
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
		});

	// set camera orientation
	// TODO: this could be a function generator
	input.bind(modes::Move,
		[&, game] (SDL_Event& ev, unsigned flags) {
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
		});

	input.setMode(modes::Move);
}

void playerView::handleInput(gameMain *game, SDL_Event& ev) {
	input.dispatch(ev);
}

void playerView::logic(gameMain *game, float delta) {
	// TODO: cam->update(delta);
	/*
	cam->position += cam->velocity.z*cam->direction*delta;
	cam->position += cam->velocity.y*cam->up*delta;
	cam->position += cam->velocity.x*cam->right*delta;
	*/
	game->phys->set_acceleration(cameraPhysID, cam->velocity);
	cam->position = cameraObj->transform.position + glm::vec3(0, 1.5, 0);
	cameraObj->transform.rotation = glm::quat(glm::vec3(
		// pitch, yaw, roll
		cam->direction.y*-0.5f,
		atan2(cam->direction.x, cam->direction.z),
		0));
}

void playerView::render(gameMain *game) {
	if (!testpost) {
		// XXX: keep postprocessing chain in renderer class
		testpost = makePostprocessor<rOutput>(game->rend->shaders["post"],
		                                      SCREEN_SIZE_X, SCREEN_SIZE_Y);
	}

	// TODO: draw player UI stuff (text, etc)
	// TODO: handle this in gameMain or handleInput here
	// TODO: this is mostly shared with game_editor::render(), how
	//       to best abstract this...
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	game->rend->framebuffer->clear();

	if (game->state->rootnode) {
		renderQueue que(cam);
		que.add(game->state->rootnode);
		que.updateLights(game->rend->shaders["shadow"], game->rend->atlases);
		que.updateReflections(game->rend->shaders["refprobe"],
		                      game->rend->atlases,
		                      game->rend->defaultSkybox);
		que.add(game->state->physObjects);
		DO_ERROR_CHECK();

		game->rend->framebuffer->framebuffer->bind();
		DO_ERROR_CHECK();

		game->rend->shaders["main"]->bind();
		glActiveTexture(GL_TEXTURE6);
		game->rend->atlases.reflections->color_tex->bind();
		game->rend->shaders["main"]->set("reflection_atlas", 6);
		glActiveTexture(GL_TEXTURE7);
		game->rend->atlases.shadows->depth_tex->bind();
		game->rend->shaders["main"]->set("shadowmap_atlas", 7);
		DO_ERROR_CHECK();

		game->rend->shaders["main"]->set("time_ms", SDL_GetTicks() * 1.f);
		DO_ERROR_CHECK();
		
		que.flush(game->rend->framebuffer, game->rend->shaders["main"],
		          game->rend->atlases);

		game->rend->defaultSkybox.draw(cam, game->rend->framebuffer);
	}
	
	testpost->setSize(winsize_x, winsize_y);
	testpost->draw(game->rend->framebuffer);

	glDepthMask(GL_TRUE);
	enable(GL_DEPTH_TEST);
}
