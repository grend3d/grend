#include <grend/gameMain.hpp>
#include <grend/gameMainDevWindow.hpp>
#include <grend/gameObject.hpp>
//#include <grend/playerView.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>
#include <grend/controllers.hpp>

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
#define LOCAL_BUILD 1

using namespace grendx;
using namespace grendx::ecs;

#include "player.hpp"
#include "enemy.hpp"
#include "inputHandler.hpp"
#include "landscapeGenerator.hpp"
#include "projectile.hpp"
#include "health.hpp"
#include "healthbar.hpp"

class landscapeGenView : public gameView {
	public:
		typedef std::shared_ptr<landscapeGenView> ptr;
		typedef std::weak_ptr<landscapeGenView>   weakptr;

		landscapeGenView(gameMain *game);
		virtual void logic(gameMain *game, float delta);
		virtual void render(gameMain *game);
		//void loadPlayer(void);

		enum modes {
			MainMenu,
			Move,
			Pause,
		};

		renderPostChain::ptr post = nullptr;
		//modalSDLInput input;
		vecGUI vgui;
		int menuSelect = 0;

		entityManager::ptr manager;
		landscapeGenerator landscape;
};

landscapeGenView::landscapeGenView(gameMain *game) : gameView() {
	post = renderPostChain::ptr(new renderPostChain(
				{game->rend->postShaders["tonemap"], game->rend->postShaders["psaa"]},
				SCREEN_SIZE_X, SCREEN_SIZE_Y));
	manager = std::make_shared<entityManager>(game);

	// TODO: names are kinda pointless here
	manager->systems["collision"] =
		std::make_shared<entitySystemCollision>();

	manager->systems["syncPhysics"] =
		std::make_shared<syncRigidBodySystem>();

	auto inputSys = std::make_shared<inputHandlerSystem>();
	manager->systems["input"] = inputSys;

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
	input.bind(MODAL_ALL_MODES, resizeInputHandler(game, post));
	input.bind(modes::Move, controller::camMovement(cam, 10.f));
	input.bind(modes::Move, controller::camFPS(cam, game));
	input.bind(modes::Move, inputMapper(inputSys->inputs, cam));
	input.setMode(modes::Move);
};

void landscapeGenView::logic(gameMain *game, float delta) {
	static glm::vec3 lastvel = glm::vec3(0);
	static gameObject::ptr retval;

	if (cam->velocity() != lastvel) {
		lastvel = cam->velocity();
	}

	game->phys->stepSimulation(delta);
	game->phys->filterCollisions();;
	
	entity *playerEnt = findFirst(manager.get(), {"player"});

	if (playerEnt) {
		TRS& transform = playerEnt->getNode()->transform;
		cam->setPosition(transform.position - 5.f*cam->direction());
		landscape.setPosition(game, transform.position);
	}

	manager->update(delta);
}

static void renderHealthbars(entityManager *manager,
                             vecGUI& vgui,
                             camera::ptr cam)
{
	std::set<entity*> ents = searchEntities(manager, {"health", "healthbar"});

	for (auto& ent : ents) {
		healthbar *bar;
		castEntityComponent(bar, manager, ent, "healthbar");

		if (bar) {
			bar->draw(manager, ent, vgui, cam);
		}
	}
}

void landscapeGenView::render(gameMain *game) {
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);
	renderFlags flags = game->rend->getLightingFlags();

	if (input.mode == modes::MainMenu) {
		renderWorld(game, cam, flags);

		// TODO: need to set post size on resize event..
		//post->setSize(winsize_x, winsize_y);
		post->draw(game->rend->framebuffer);
		input.setMode(modes::Move);

		// TODO: function to do this
		//drawMainMenu(winsize_x, winsize_y);

	} else {
		renderWorld(game, cam, flags);
		post->draw(game->rend->framebuffer);

		Framebuffer().bind();
		setDefaultGlFlags();

		disable(GL_DEPTH_TEST);
		disable(GL_SCISSOR_TEST);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		nvgBeginFrame(vgui.nvg, game->rend->screen_x, game->rend->screen_y, 1.0);
		nvgSave(vgui.nvg);

		renderHealthbars(manager.get(), vgui, cam);

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
	//gameMain *game = new gameMain();

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
