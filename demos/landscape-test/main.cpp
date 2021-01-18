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
#include <grend/ecs/serializer.hpp>
#include <grend/ecs/rigidBodySerializer.hpp>

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
#include "health.hpp"
#include "healthbar.hpp"
#include "enemyCollision.hpp"
#include "healthPickup.hpp"
#include "timedLifetime.hpp"

class landscapeEventSystem : public entitySystem {
	public:
		virtual void update(entityManager *manager, float delta);

		generatorEventQueue queue
			= std::make_shared<std::vector<generatorEvent>>();
};

class generatorEventHandler : public component {
	public:
		generatorEventHandler(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "generatorEventHandler", this);
		}

		virtual void
		handleEvent(entityManager *manager, entity *ent, generatorEvent& ev) {
			std::cerr << "handleEvent(): got here, " << "("
				<< ev.position.x << "[+/-" << ev.extent.x << "], "
				<< ev.position.y << "[+/-" << ev.extent.y << "], "
				<< ev.position.z << "[+/-" << ev.extent.z << "]"
				<< ") was ";

			switch (ev.type) {
				case generatorEvent::types::generatorStarted:
					std::cerr << "started";
					break;

				case generatorEvent::types::generated:
					std::cerr << "generated";
					break;

				case generatorEvent::types::deleted:
					std::cerr << "deleted";
					break;

				default:
					std::cerr << "<unknown>";
					break;
			}

			std::cerr << std::endl;
		}
};

class generatorEventActivator : public generatorEventHandler {
	public:
		generatorEventActivator(entityManager *manager, entity *ent)
			: generatorEventHandler(manager, ent)
		{
			manager->registerComponent(ent, "generatorEventActivator", this);
		}

		virtual void
		handleEvent(entityManager *manager, entity *ent, generatorEvent& ev) {
			std::cerr << "TODO: activate/deactivate stuff here" << std::endl;
		}
};

void landscapeEventSystem::update(entityManager *manager, float delta) {
	auto handlers = manager->getComponents("generatorEventHandler");

	for (auto& ev : *queue) {
		for (auto& it : handlers) {
			generatorEventHandler *handler = dynamic_cast<generatorEventHandler*>(it);
			entity *ent = manager->getEntity(handler);

			if (handler && ent) {
				handler->handleEvent(manager, ent, ev);
			}
		}
	}

	queue->clear();
}

// XXX: this sort of makes sense but not really... game entity with no renderable
//      objects, functioning as basically a subsystem? 
//      abstraction here doesn't make sense, needs to be redone
class worldEntityGenerator : public generatorEventHandler {
	public:
		worldEntityGenerator(entityManager *manager, entity *ent)
			: generatorEventHandler(manager, ent)
		{
			manager->registerComponent(ent, "generatorEventHandler", this);
		}

		virtual void
		handleEvent(entityManager *manager, entity *ent, generatorEvent& ev) {
			switch (ev.type) {
				case generatorEvent::types::generated:
					{
						// XXX
						std::tuple<float, float, float> foo =
							{ ev.position.x, ev.position.y, ev.position.z };

						if (positions.count(foo) == 0) {
							positions.insert(foo);
							std::cerr
								<< "worldEntityGenerator(): generating some things"
								<< std::endl;

							manager->add(new enemy(manager, manager->engine, ev.position + glm::vec3(0, 50.f, 0)));

							// TODO: need a way to know what the general shape of
							//       the generated thing is...
							manager->add(new healthPickup(manager, ev.position + glm::vec3(0, 10.f, 0)));
						}
					}
					break;

				case generatorEvent::types::deleted:
					break;

				default:
					break;
			}
		}

		std::set<std::tuple<float, float, float>> positions;
};

class worldEntitySpawner : public entity {
	public:
		worldEntitySpawner(entityManager *manager)
			: entity (manager)
		{
			manager->registerComponent(this, "worldEntitySpawner", this);
			new worldEntityGenerator(manager, this);
		}

		virtual void update(entityManager *manager, float delta) { /* nop */ };
};

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
		float zoom = 5.f;

		landscapeGenerator landscape;
};

landscapeGenView::landscapeGenView(gameMain *game) : gameView() {
	post = renderPostChain::ptr(new renderPostChain(
				{game->rend->postShaders["tonemap"], game->rend->postShaders["psaa"]},
				SCREEN_SIZE_X, SCREEN_SIZE_Y));

	game->serializers->add<rigidBodySphereSerializer>();
	game->serializers->add<syncRigidBodyTransformSerializer>();
	game->serializers->add<syncRigidBodyPositionSerializer>();
	game->serializers->add<syncRigidBodyXZVelocitySerializer>();

	// TODO: names are kinda pointless here
	// TODO: should systems be a state object in gameMain as well?
	//       they practically are since the entityManager here is, just one
	//       level deep...
	game->entities->systems["lifetime"] = std::make_shared<lifetimeSystem>();
	game->entities->systems["collision"] = std::make_shared<entitySystemCollision>();
	game->entities->systems["syncPhysics"] = std::make_shared<syncRigidBodySystem>();

	auto inputSys = std::make_shared<inputHandlerSystem>();
	game->entities->systems["input"] = inputSys;

	auto generatorSys = std::make_shared<landscapeEventSystem>();
	game->entities->systems["landscapeEvents"] = generatorSys;
	landscape.setEventQueue(generatorSys->queue);

	//manager->add(new player(manager.get(), game, glm::vec3(-15, 50, 0)));
	player *playerEnt = new player(game->entities.get(), game, glm::vec3(0, 20, 0));
	game->entities->add(playerEnt);
	new generatorEventHandler(game->entities.get(), playerEnt);
	new health(game->entities.get(), playerEnt);
	new enemyCollision(game->entities.get(), playerEnt);
	new healthPickupCollision(game->entities.get(), playerEnt);

	game->entities->add(new worldEntitySpawner(game->entities.get()));

	/*
	for (unsigned i = 0; i < 15; i++) {
		glm::vec3 position = glm::vec3(
			float(rand()) / RAND_MAX * 100.0 - 50,
			50.0,
			float(rand()) / RAND_MAX * 100.0 - 50
		);

		manager->add(new enemy(manager.get(), game, position));
	}
	*/

	bindCookedMeshes();
	input.bind(MODAL_ALL_MODES, resizeInputHandler(game, post));
	input.bind(modes::Move, controller::camMovement(cam, 10.f));
	//input.bind(modes::Move, controller::camFPS(cam, game));
	input.bind(modes::Move, controller::camAngled2DFixed(cam, game, -M_PI/4.f));
	input.bind(modes::Move, controller::camScrollZoom(cam, &zoom));
	input.bind(modes::Move, inputMapper(inputSys->inputs, cam));
	input.bind(MODAL_ALL_MODES,
		[&, this] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_h) {
				std::cerr << "Got here! serializing stuff" << std::endl;
				std::cerr
					<< game->serializers->serializeEntities((game->entities.get())).dump(4)
					<< std::endl;
			}

			return MODAL_NO_CHANGE;
		});
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
	
	entity *playerEnt = findFirst(game->entities.get(), {"player"});

	while (!playerEnt) {
		playerEnt = new player(game->entities.get(), game, glm::vec3(0, 20, 0));
		game->entities->add(playerEnt);
		new generatorEventHandler(game->entities.get(), playerEnt);
		new health(game->entities.get(), playerEnt);
		new enemyCollision(game->entities.get(), playerEnt);
		new healthPickupCollision(game->entities.get(), playerEnt);
	}

	if (playerEnt) {
		TRS& transform = playerEnt->getNode()->transform;
		cam->setPosition(transform.position - zoom*cam->direction());
		landscape.setPosition(game, transform.position);
	}

	game->entities->update(delta);
}

static void drawPlayerHealthbar(entityManager *manager,
                                vecGUI&vgui,
                                health *playerHealth)
{
	int wx = manager->engine->rend->screen_x;
	int wy = manager->engine->rend->screen_y;

	nvgBeginPath(vgui.nvg);
	nvgRoundedRect(vgui.nvg, 48, 35, 16, 42, 3);
	nvgRoundedRect(vgui.nvg, 35, 48, 42, 16, 3);
	nvgFillColor(vgui.nvg, nvgRGBA(172, 16, 16, 192));
	nvgFill(vgui.nvg);

	//nvgRotate(vgui.nvg, 0.1*cos(ticks));
	nvgBeginPath(vgui.nvg);
	nvgRect(vgui.nvg, 90, 44, 256, 24);
	nvgFillColor(vgui.nvg, nvgRGBA(30, 30, 30, 127));
	nvgFill(vgui.nvg);

	nvgBeginPath(vgui.nvg);
	nvgRect(vgui.nvg, 93, 47, 252*playerHealth->amount, 20);
	nvgFillColor(vgui.nvg, nvgRGBA(192, 32, 32, 127));
	nvgFill(vgui.nvg);
	//nvgRotate(vgui.nvg, -0.1*cos(ticks));

	nvgBeginPath(vgui.nvg);
	nvgRoundedRect(vgui.nvg, wx - 250, 50, 200, 100, 10);
	nvgFillColor(vgui.nvg, nvgRGBA(28, 30, 34, 192));
	nvgFill(vgui.nvg);

	nvgFontSize(vgui.nvg, 16.f);
	nvgFontFace(vgui.nvg, "sans-bold");
	nvgFontBlur(vgui.nvg, 0);
	nvgTextAlign(vgui.nvg, NVG_ALIGN_LEFT);
	nvgFillColor(vgui.nvg, nvgRGBA(0xf0, 0x60, 0x60, 160));
	nvgText(vgui.nvg, wx - 82, 80, "❎", NULL);
	nvgFillColor(vgui.nvg, nvgRGBA(220, 220, 220, 160));
	nvgText(vgui.nvg, wx - 235, 80, "💚 Testing this", NULL);
	nvgText(vgui.nvg, wx - 235, 80 + 16, "Go forward ➡", NULL);
	nvgText(vgui.nvg, wx - 235, 80 + 32, "⬅ Go back", NULL);
}

static void renderHealthbars(entityManager *manager,
                             vecGUI& vgui,
                             camera::ptr cam)
{
	std::set<entity*> ents = searchEntities(manager, {"health", "healthbar"});
	std::set<entity*> players = searchEntities(manager, {"player", "health"});

	for (auto& ent : ents) {
		healthbar *bar;
		castEntityComponent(bar, manager, ent, "healthbar");

		if (bar) {
			bar->draw(manager, ent, vgui, cam);
		}
	}

	if (players.size() > 0) {
		entity *ent = *players.begin();
		health *playerHealth;

		castEntityComponent(playerHealth, manager, ent, "health");

		if (playerHealth) {
			drawPlayerHealthbar(manager, vgui, playerHealth);
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

		renderHealthbars(game->entities.get(), vgui, cam);

		nvgRestore(vgui.nvg);
		nvgEndFrame(vgui.nvg);
	}
}

int main(int argc, char *argv[]) {
	std::cerr << "entering main()" << std::endl;
	std::cerr << "started SDL context" << std::endl;
	std::cerr << "have game state" << std::endl;

	try {
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
		landscapeMaterial->factors.metalness = 1.f;
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

		setNode("entities",  game->state->rootnode, game->entities->root);
		setNode("landscape", game->state->rootnode, player->landscape.getNode());

		game->run();

	} catch (const std::exception& ex) {
		std::cerr << "Exception! " << ex.what() << std::endl;

	} catch (const char* ex) {
		std::cerr << "Exception! " << ex << std::endl;
	}

	return 0;
}
