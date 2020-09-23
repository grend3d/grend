#include <grend/playerView.hpp>
#include <grend/gameView.hpp>
#include <grend/audioMixer.hpp>
#include <glm/gtx/rotate_vector.hpp>

#if GLSL_VERSION < 300
// GLES2
#define NANOVG_GLES2
#define NANOVG_GLES2_IMPLEMENTATION
#elif GLSL_VERSION == 300
// GLES3
#define NANOVG_GLES3
#define NANOVG_GLES3_IMPLEMENTATION
#else
// GL3+ core
#define NANOVG_GL3
#define NANOVG_GL3_IMPLEMENTATION
#endif

#include <nanovg/src/nanovg.h>
#include <nanovg/src/nanovg_gl.h>
#include <nanovg/src/nanovg_gl_utils.h>

using namespace grendx;

static renderPostStage<rOutput>::ptr testpost = nullptr;
static gameObject::ptr testweapon = nullptr;

static channelBuffers_ptr weaponSound =
	openAudio("test-assets/obj/sounds/Audio/footstep_snow_003.ogg");

struct nvg_data {
	int fontNormal, fontBold, fontIcons, fontEmoji;
};

playerView::playerView(gameMain *game) : gameView() {
	static const float speed = 15.f;
#if GLSL_VERSION < 300
	//vg = nvgCreateGLES2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
#else
	//vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
	vg = nvgCreateGL3(NVG_ANTIALIAS);
#endif
	assert(vg != nullptr);
	struct nvg_data temp;
	temp.fontNormal = nvgCreateFont(vg, "sans", "libs/nanovg/example/Roboto-Regular.ttf");
	temp.fontBold = nvgCreateFont(vg, "sans-bold", "libs/nanovg/example/Roboto-Bold.ttf");
	temp.fontEmoji = nvgCreateFont(vg, "emoji", "libs/nanovg/example/NotoEmoji-Regular.ttf");
	nvgAddFallbackFontId(vg, temp.fontNormal, temp.fontEmoji);
	nvgAddFallbackFontId(vg, temp.fontBold, temp.fontEmoji);

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

	// set camera orientation
	// TODO: this could be a function generator
	input.bind(modes::Move,
		[&, game] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_MOUSEBUTTONDOWN
			    && ev.button.button == SDL_BUTTON_LEFT)
			{
				auto sound = openAudio("test-assets/obj/sounds/Audio/footstep_snow_003.ogg");
				auto ptr = std::make_shared<stereoAudioChannel>(sound);
				game->audio->add(ptr);
			}

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

	renderWorld(game, cam);

	// TODO: function to do this
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);
	testpost->setSize(winsize_x, winsize_y);
	testpost->draw(game->rend->framebuffer);
	
	Framebuffer().bind();
	nvgBeginFrame(vg, winsize_x, winsize_y, 1.0);
	nvgSave(vg);

	float ticks = SDL_GetTicks() / 1000.f;
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	nvgBeginPath(vg);
	nvgRoundedRect(vg, 48, 35, 16, 42, 3);
	nvgRoundedRect(vg, 35, 48, 42, 16, 3);
	nvgFillColor(vg, nvgRGBA(172, 16, 16, 192));
	nvgFill(vg);

	nvgRotate(vg, 0.1*cos(ticks));
	nvgBeginPath(vg);
	nvgRect(vg, 90, 44, 256, 24);
	nvgFillColor(vg, nvgRGBA(30, 30, 30, 127));
	nvgFill(vg);

	nvgBeginPath(vg);
	nvgRect(vg, 93, 47, 128 + 120*sin(ticks), 20);
	nvgFillColor(vg, nvgRGBA(192, 32, 32, 127));
	nvgFill(vg);

	nvgRotate(vg, -0.1*cos(ticks));
	nvgBeginPath(vg);
	nvgRoundedRect(vg, winsize_x - 250, 50, 200, 100, 10);
	nvgFillColor(vg, nvgRGBA(28, 30, 34, 192));
	nvgFill(vg);

	nvgFontSize(vg, 16.f);
	nvgFontFace(vg, "sans-bold");
	nvgFontBlur(vg, 0);
	nvgFillColor(vg, nvgRGBA(220, 220, 220, 160));
	nvgTextAlign(vg, NVG_ALIGN_LEFT);
	nvgText(vg, winsize_x - 235, 80, "💚 Testing this", NULL);
	nvgText(vg, winsize_x - 235, 80 + 16, "Go forward ➡", NULL);
	nvgText(vg, winsize_x - 235, 80 + 32, "⬅ Go back", NULL);

	nvgRestore(vg);
	nvgEndFrame(vg);
}
