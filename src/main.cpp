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

#include <memory>
#include <chrono>
#include <map>
#include <vector>
#include <set>
#include <functional>

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
#include "UI.hpp"
#include "flag.hpp"
#include "team.hpp"
#include "enemySpawner.hpp"
#include "levelController.hpp"

class landscapeEventSystem : public entitySystem {
	public:
		virtual void update(entityManager *manager, float delta);

		generatorEventQueue::ptr queue = std::make_shared<generatorEventQueue>();
};

class abyssDeleter : public entitySystem {
	public:
		virtual void update(entityManager *manager, float delta) {
			// XXX: delete entities that fall into the abyss
			//      could be done more efficiently,
			//      ie. some sort of spatial partitioning
			for (auto& ent : manager->entities) {
				if (ent->node->transform.position.y < -100.f) {
					manager->remove(ent);
				}
			}
		}
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
			const char *typestr;

			switch (ev.type) {
				case generatorEvent::types::generatorStarted:
					typestr =  "started";
					break;

				case generatorEvent::types::generated:
					typestr =  "generated";
					break;

				case generatorEvent::types::deleted:
					typestr =  "deleted";
					break;

				default:
					typestr =  "<unknown>";
					break;
			}

			SDL_Log(
				"handleEvent: got here, %s [+/-%g] [+/-%g] [+/-%g]",
				typestr, ev.extent.x, ev.extent.y, ev.extent.z);

			/*

			std::cerr << std::endl;
			*/
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
			// TODO: activate/deactivate stuff here
		}
};

void landscapeEventSystem::update(entityManager *manager, float delta) {
	auto handlers = manager->getComponents("generatorEventHandler");
	auto g = queue->lock();
	auto& quevec = queue->getQueue();

	for (auto& ev : quevec) {
		for (auto& it : handlers) {
			generatorEventHandler *handler = dynamic_cast<generatorEventHandler*>(it);
			entity *ent = manager->getEntity(handler);

			if (handler && ent) {
				handler->handleEvent(manager, ent, ev);
			}
		}
	}

	// XXX: should be in queue class
	quevec.clear();
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
							SDL_Log("worldEntityGenerator(): generating some things");

							//manager->add(new enemy(manager, manager->engine, ev.position + glm::vec3(0, 50.f, 0)));

							// TODO: need a way to know what the general shape of
							//       the generated thing is...
							//manager->add(new healthPickup(manager, ev.position + glm::vec3(0, 10.f, 0)));
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

class projalphaView : public gameView {
	public:
		typedef std::shared_ptr<projalphaView> ptr;
		typedef std::weak_ptr<projalphaView>   weakptr;

		projalphaView(gameMain *game);
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
		float zoom = 20.f;

		std::unique_ptr<levelController> level;
		landscapeGenerator landscape;
		inputHandlerSystem::ptr inputSystem;

	private:
		void drawMainMenu(int wx, int wy);

};

// XXX
static glm::vec2 movepos(0, 0);
static glm::vec2 actionpos(0, 0);

projalphaView::projalphaView(gameMain *game)
	: gameView(),
	  level(new levelController)
{
	post = renderPostChain::ptr(new renderPostChain(
		{loadPostShader(GR_PREFIX "shaders/src/texpresent.frag",
		                game->rend->globalShaderOptions)},
		//{game->rend->postShaders["tonemap"], game->rend->postShaders["psaa"]},
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
	game->entities->systems["area"]     = std::make_shared<areaSystem>();
	game->entities->systems["abyss"]    = std::make_shared<abyssDeleter>();
	game->entities->systems["collision"] = std::make_shared<entitySystemCollision>();
	game->entities->systems["syncPhysics"] = std::make_shared<syncRigidBodySystem>();

	inputSystem = std::make_shared<inputHandlerSystem>();
	game->entities->systems["input"] = inputSystem;

	auto generatorSys = std::make_shared<landscapeEventSystem>();
	game->entities->systems["landscapeEvents"] = generatorSys;
	landscape.setEventQueue(generatorSys->queue);

	bindCookedMeshes();
	input.bind(MODAL_ALL_MODES, resizeInputHandler(game, post));

#if !defined(__ANDROID__)
	//input.bind(modes::Move, controller::camMovement(cam, 30.f));
	//input.bind(modes::Move, controller::camFPS(cam, game));
	input.bind(modes::Move, controller::camMovement2D(cam, 15.f));
	input.bind(modes::Move, controller::camScrollZoom(cam, &zoom));
	input.bind(modes::Move, inputMapper(inputSystem->inputs, cam));
#endif

	input.bind(modes::Move, controller::camAngled2DFixed(cam, game, -M_PI/4.f));
	input.bind(modes::Move, [=] (SDL_Event& ev, unsigned flags) {
		inputSystem->handleEvent(game->entities.get(), ev);
		return MODAL_NO_CHANGE;
	});

	input.bind(MODAL_ALL_MODES,
		[&, this] (SDL_Event& ev, unsigned flags) {
			if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_h) {
				// TODO: log interface
				/*
				std::cerr << "Got here! serializing stuff" << std::endl;
				std::cerr
					<< game->serializers->serializeEntities((game->entities.get())).dump(4)
					<< std::endl;
					*/
			}

			return MODAL_NO_CHANGE;
		});

	input.setMode(modes::MainMenu);
};

void projalphaView::logic(gameMain *game, float delta) {
	if (input.mode != modes::Move) {
		// XXX:
		return;
	}

	static glm::vec3 lastvel = glm::vec3(0);
	static gameObject::ptr retval;

	if (cam->velocity() != lastvel) {
		lastvel = cam->velocity();
	}

	game->phys->stepSimulation(delta);
	game->phys->filterCollisions();;

	entity *playerEnt = findFirst(game->entities.get(), {"player"});

	if (playerEnt) {
		TRS& transform = playerEnt->getNode()->transform;
		cam->slide(transform.position - zoom*cam->direction(), 8.f, delta);
	}

	game->entities->update(delta);

	if (level->won()) {
		SDL_Log("winner winner, a dinner of chicken!");
		level->reset();
	} 

	auto lost = level->lost();
	if (lost.first) {
		SDL_Log("lol u died: %s", lost.second.c_str());
		input.setMode(modes::MainMenu);
	}
}

void projalphaView::render(gameMain *game) {
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);
	renderFlags flags = game->rend->getLightingFlags();

	if (input.mode == modes::MainMenu) {
		renderWorld(game, cam, flags);

		// TODO: need to set post size on resize event..
		//post->setSize(winsize_x, winsize_y);
		post->draw(game->rend->framebuffer);
		//input.setMode(modes::Move);

		// TODO: function to do this
		drawMainMenu(winsize_x, winsize_y);

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
		renderObjectives(game->entities.get(), level.get(), vgui);
		renderControls(game, vgui);

		nvgRestore(vgui.nvg);
		nvgEndFrame(vgui.nvg);
	}
}

void projalphaView::drawMainMenu(int wx, int wy) {
	static int selected;
	bool reset = false;

	vgui.newFrame(wx, wy);
	vgui.menuBegin(wx / 2 - 100, wy / 2 - 100, 200, "testing");

	if (vgui.menuEntry("Play", &selected)) {
		if (vgui.clicked()) {
			reset = true; // XXX:
			input.setMode(modes::Move);
		}
	}

	vgui.menuEntry("Quit", &selected);
	vgui.menuEnd();
	vgui.endFrame();

	if (reset) {
		level->reset();
	}
}

void initEntitiesFromNodes(gameObject::ptr node,
                           std::function<void(const std::string&, gameObject::ptr&)> init)
{
	if (node == nullptr) {
		return;
	}

	for (auto& [name, ptr] : node->nodes) {
		init(name, ptr);
	}
}

int main(int argc, const char *argv[]) try {
	const char *mapfile = "assets/maps/base.map";

	if (argc > 1) {
		mapfile = argv[1];
	}

	SDL_Log("entering main()");
	SDL_Log("started SDL context");
	SDL_Log("have game state");

	TRS staticPosition; // default
	gameMain *game = new gameMainDevWindow();

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

	game->state->rootnode = loadMap(game, mapfile);
	game->phys->addStaticModels(nullptr, game->state->rootnode, staticPosition);

	projalphaView::ptr view = std::make_shared<projalphaView>(game);
	view->cam->setFar(1000.0);
	view->cam->setFovx(70.0);
	game->setView(view);
	game->rend->lightThreshold = 0.1;

	// JS flashbacks
	view->level->addInit([=] () {
		gameObject::ptr flagnodes = game->state->rootnode->getNode("flags");

		initEntitiesFromNodes(flagnodes,
			[=] (const std::string& name, gameObject::ptr& ptr) {
				std::cerr << "have flag node " << name << std::endl;

				auto f = new flag(game->entities.get(), game,
								  ptr->transform.position, name);
				game->entities->add(f);
			});
	});

	view->level->addInit([=] () {
		gameObject::ptr spawners = game->state->rootnode->getNode("spawners");

		initEntitiesFromNodes(spawners,
			[&] (const std::string& name, gameObject::ptr& ptr) {
				std::cerr << "have spawner node " << name << std::endl;

				auto en = new enemySpawner(game->entities.get(), game,
										   ptr->transform.position);
				new team(game->entities.get(), en, "red");
				game->entities->add(en);
			});
	});

	view->level->addInit([=] () {
		entity *playerEnt;

		playerEnt = new player(game->entities.get(), game, glm::vec3(-5, 20, -5));
		game->entities->add(playerEnt);
		new generatorEventHandler(game->entities.get(), playerEnt);
		new health(game->entities.get(), playerEnt);
		new enemyCollision(game->entities.get(), playerEnt);
		new healthPickupCollision(game->entities.get(), playerEnt);
		new flagPickup(game->entities.get(), playerEnt);
		new team(game->entities.get(), playerEnt, "blue");

#if defined(__ANDROID__)
		int wx = game->rend->screen_x;
		int wy = game->rend->screen_y;
		glm::vec2 movepad  ( 2*wx/16.f, 7*wy/9.f);
		glm::vec2 actionpad(14*wx/16.f, 7*wy/9.f);

		new touchMovementHandler(game->entities.get(), playerEnt, cam,
								 view->inputSystem->inputs, movepad, 150.f);
		new touchRotationHandler(game->entities.get(), playerEnt, cam,
								 view->inputSystem->inputs, actionpad, 150.f);

#else
		new mouseRotationPoller(game->entities.get(), playerEnt);
#endif
	});

	view->level->addDestructor([=] () {
		// TODO: should just have reset function in entity manager
		for (auto& ent : game->entities->entities) {
			game->entities->remove(ent);
		}

		game->entities->clearFreedEntities();
	});

	view->level->addObjective("Destroy all robospawners",
		[=] () {
			std::set<entity*> spawners
				= searchEntities(game->entities.get(), {"enemySpawner"});

			return spawners.size() == 0;
		});

	view->level->addObjective("Destroy all enemy robots",
		[=] () {
			std::set<entity*> enemies
				= searchEntities(game->entities.get(), {"enemy"});

			return enemies.size() == 0;
		});

	view->level->addLoseCondition(
		[=] () {
			std::set<entity*> players
				= searchEntities(game->entities.get(), {"player"});

			return std::pair<bool, std::string>(players.size() == 0, "lol u died");
		});

	setNode("entities",  game->state->rootnode, game->entities->root);
	SDL_Log("Got to game->run()!");
	game->run();

	return 0;

} catch (const std::exception& ex) {
	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Exception! %s", ex.what());

} catch (const char* ex) {
	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Exception! %s", ex);
}
