#include <grend/gameMainDevWindow.hpp>
#include <grend/gameMainWindow.hpp>
#include <grend/gameObject.hpp>
#include <grend/playerView.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/collision.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <memory>
#include <chrono>
#include <map>
#include <vector>
#include <set>

#include <initializer_list>

// XXX:  toggle using textures/models I have locally, don't want to
//       bloat the assets folder again
#define LOCAL_BUILD 0

using namespace grendx;
using namespace grendx::ecs;

#include "player.hpp"
#include "enemy.hpp"
#include "inputHandler.hpp"
#include "landscapeGenerator.hpp"
#include "projectile.hpp"

class landscapeGenView : public playerView {
	public:
		typedef std::shared_ptr<landscapeGenView> ptr;
		typedef std::weak_ptr<landscapeGenView>   weakptr;

		landscapeGenView(gameMain *game) : playerView(game) {
			manager = std::make_shared<entityManager>(game);

			// TODO: names are kinda pointless here
			manager->systems["collision"] =
				std::make_shared<entitySystemCollision>();

			auto inputSys = std::make_shared<inputHandlerSystem>();
			manager->systems["input"] = inputSys;
			input.bind(modes::Move, inputMapper(inputSys->inputs, cam));

			manager->add(new player(manager.get(), game, glm::vec3(-15, 50, 0)));
			player *playerEnt = new player(manager.get(), game, glm::vec3(0, 20, 0));
			manager->add(playerEnt);

			for (unsigned i = 0; i < 15; i++) {
				glm::vec3 position = glm::vec3(
					float(rand()) / RAND_MAX * 100.0 - 50,
					50.0,
					float(rand()) / RAND_MAX * 100.0 - 50
				);

				manager->add(new enemy(manager.get(), game, position));
			}

			bindCookedMeshes();
		};

		virtual void logic(gameMain *game, float delta);
		virtual void render(gameMain *game);
		//void loadPlayer(void);

		entityManager::ptr manager;
		landscapeGenerator landscape;
};

void landscapeGenView::logic(gameMain *game, float delta) {
	static glm::vec3 lastvel = glm::vec3(0);
	static gameObject::ptr retval;

	if (cam->velocity() != lastvel) {
		lastvel = cam->velocity();
	}

	game->phys->stepSimulation(1.f/game->frame_timer.last());
	game->phys->filterCollisions();;
	
	entity *playerEnt = findFirst(manager.get(), {"player", "entity"});

	if (playerEnt) {
		TRS& transform = playerEnt->getNode()->transform;
		cam->setPosition(transform.position - 5.f*cam->direction());
		landscape.setPosition(game, transform.position);
	}

	manager->update(1.f / game->frame_timer.last());
}

void landscapeGenView::render(gameMain *game) {
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	if (input.mode == modes::MainMenu) {
		renderWorld(game, cam);

		// TODO: need to set post size on resize event..
		//post->setSize(winsize_x, winsize_y);
		post->draw(game->rend->framebuffer);
		input.setMode(modes::Move);

		// TODO: function to do this
		//drawMainMenu(winsize_x, winsize_y);

	} else {
		renderWorld(game, cam);
		post->draw(game->rend->framebuffer);

		Framebuffer().bind();
		setDefaultGlFlags();

		disable(GL_DEPTH_TEST);
		disable(GL_SCISSOR_TEST);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		nvgBeginFrame(vgui.nvg, game->rend->screen_x, game->rend->screen_y, 1.0);
		nvgSave(vgui.nvg);

		/*
		for (unsigned i = 0; i < enemies.size(); i++) {
			auto& ptr = enemies[i];
			float ticks = SDL_GetTicks() / 1000.f;
			glm::vec4 pos = cam->worldToScreenPosition(ptr->transform.position + glm::vec3(0, 10, 0));

			if (cam->onScreen(pos)) {
				// TODO: some sort of grid editor wouldn't be too hard,
				//       probably worthwhile for quick UIs
				float depth = 16*max(0.f, pos.w);
				float pad = depth*16.f;

				float width  = 8*pad;
				float height = 6*pad;

				pos.y  = 1.0 - pos.y;
				pos.x *= game->rend->screen_x;
				pos.y *= game->rend->screen_y;

				glm::vec2 outer    = glm::vec2(pos) - glm::vec2(width, height)*0.5f;
				glm::vec2 innermin = outer + pad;
				glm::vec2 innermax = outer + glm::vec2(width, height) - 2*pad;

				nvgFontSize(vgui.nvg, pad);
				nvgFontFace(vgui.nvg, "sans-bold");
				nvgFontBlur(vgui.nvg, 0);
				nvgTextAlign(vgui.nvg, NVG_ALIGN_LEFT);

				nvgBeginPath(vgui.nvg);
				nvgRect(vgui.nvg, outer.x, outer.y, width, height);
				nvgFillColor(vgui.nvg, nvgRGBA(28, 30, 34, 192));
				nvgFill(vgui.nvg);

				float amount = sin(i*ticks)*0.5 + 0.5;
				nvgBeginPath(vgui.nvg);
				nvgRect(vgui.nvg, innermin.x, innermin.y, amount*(width - 2*pad), pad);
				nvgFillColor(vgui.nvg, nvgRGBA(0, 192, 0, 192));
				nvgFill(vgui.nvg);

				nvgBeginPath(vgui.nvg);
				nvgRect(vgui.nvg, innermin.x + amount*(width - 2*pad),
				        innermin.y, (1.f - amount)*(width - 2*pad), pad);
				nvgFillColor(vgui.nvg, nvgRGBA(192, 0, 0, 192));
				nvgFill(vgui.nvg);

				nvgFillColor(vgui.nvg, nvgRGBA(0xf0, 0x60, 0x60, 160));
				//nvgText(vgui.nvg, innermax.x, innermin.y + 1*pad, "❎", NULL);

				nvgFillColor(vgui.nvg, nvgRGBA(220, 220, 220, 160));
				nvgText(vgui.nvg, innermin.x, innermin.y + 2*pad, "💚 Enemy", NULL);
				nvgText(vgui.nvg, innermin.x, innermin.y + 3*pad, "❎ Some stats", NULL);
				nvgText(vgui.nvg, innermin.x, innermin.y + 4*pad, "❎ 123/456", NULL);
			}
		}
		*/

		nvgRestore(vgui.nvg);
		nvgEndFrame(vgui.nvg);
	}
}

int main(int argc, char *argv[]) {
	std::cerr << "entering main()" << std::endl;
	std::cerr << "started SDL context" << std::endl;
	std::cerr << "have game state" << std::endl;

	TRS staticPosition; // default
	gameMain *game = new gameMainDevWindow();

// XXX:  toggle using textures I have locally, don't want to bloat the assets
//       folder again
// TODO: include some default textures in assets for this, or procedurally generate
#if LOCAL_BUILD
	landscapeMaterial = std::make_shared<material>();
	landscapeMaterial->factors.diffuse = {1, 1, 1, 1};
	landscapeMaterial->factors.ambient = {1, 1, 1, 1};
	landscapeMaterial->factors.specular = {1, 1, 1, 1};
	landscapeMaterial->factors.emissive = {1, 1, 1, 1};
	landscapeMaterial->factors.roughness = 1.f;
	landscapeMaterial->factors.metalness = 0.f;
	landscapeMaterial->factors.opacity = 1;
	landscapeMaterial->factors.refract_idx = 1.5;

	landscapeMaterial->maps.diffuse
		= std::make_shared<materialTexture>("/home/flux/blender/tex/aerial_grass_rock_2k_jpg/aerial_grass_rock_diff_2k.jpg");
	landscapeMaterial->maps.metalRoughness
		= std::make_shared<materialTexture>("/home/flux/blender/tex/aerial_grass_rock_2k_jpg/aerial_grass_rock_rough_green_2k.jpg");
	landscapeMaterial->maps.normal
		= std::make_shared<materialTexture>("/home/flux/blender/tex/aerial_grass_rock_2k_jpg/aerial_grass_rock_nor_2k.jpg");
	landscapeMaterial->maps.ambientOcclusion
		= std::make_shared<materialTexture>("/home/flux/blender/tex/aerial_grass_rock_2k_jpg/aerial_grass_rock_ao_2k.jpg");
	landscapeMaterial->maps.emissive
		= std::make_shared<materialTexture>(GR_PREFIX "assets/tex/black.png");

	treeNode = load_object("assets-old/obj/Modular Terrain Hilly/Prop_Tree_Pine_3.obj");

#else
	landscapeMaterial = std::make_shared<material>();
	landscapeMaterial->factors.diffuse = {0.15, 0.3, 0.1, 1};
	landscapeMaterial->factors.ambient = {1, 1, 1, 1};
	landscapeMaterial->factors.specular = {1, 1, 1, 1};
	landscapeMaterial->factors.emissive = {0, 0, 0, 0};
	landscapeMaterial->factors.roughness = 0.9f;
	landscapeMaterial->factors.metalness = 0.f;
	landscapeMaterial->factors.opacity = 1;
	landscapeMaterial->factors.refract_idx = 1.5;

	treeNode = generate_cuboid(1.0, 1.0, 1.0);
#endif

	compileModel("treedude", treeNode);

	game->jobs->addAsync([=] {
		auto foo = openSpatialLoop(GR_PREFIX "assets/sfx/Bit Bit Loop.ogg");
		foo->worldPosition = glm::vec3(-10, 0, -5);
		game->audio->add(foo);
		return true;
	});

	game->jobs->addAsync([=] {
		auto bar = openSpatialLoop(GR_PREFIX "assets/sfx/Meditating Beat.ogg");
		bar->worldPosition = glm::vec3(0, 0, -5);
		game->audio->add(bar);
		return true;
	});

	game->state->rootnode = loadMap(game);
	game->phys->addStaticModels(nullptr, game->state->rootnode, staticPosition);

	landscapeGenView::ptr player = std::make_shared<landscapeGenView>(game);
	player->landscape.setPosition(game, glm::vec3(1));
	player->cam->setFar(1000.0);
	game->setView(player);

	setNode("entities", game->state->rootnode, player->manager->root);
	setNode("landscape", game->state->rootnode, player->landscape.getNode());

	game->run();
	return 0;
}
