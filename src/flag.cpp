#include "flag.hpp"
#include "team.hpp"

#include <grend/gameEditor.hpp>

flag::~flag() {};
flagPickup::~flagPickup() {};
hasFlag::~hasFlag() {};

flag::flag(entityManager *manager, gameMain *game,
           glm::vec3 position, std::string color)
	: entity(manager)
{
	new team(manager, this, color);
	new areaSphere(manager, this, 2.f);

	manager->registerComponent(this, "flag", this);

	// TODO: resource manager
	static gameObject::ptr flagModel = nullptr;
	if (!flagModel) {
		flagModel = loadSceneCompiled("assets/obj/flag.glb");
		flagModel->transform.scale = glm::vec3(2.0);
	}

	node->transform.position = position;
	setNode("model", node, flagModel);
}

void flag::update(entityManager *manager, float delta) { };

void flagPickup::onEvent(entityManager *manager, entity *ent, entity *other) {
	SDL_Log("Entered flag pickup zone!");
	team *theytem;
	team *mytem;
	castEntityComponent(mytem,   manager, ent,   "team");
	castEntityComponent(theytem, manager, other, "team");

	if (mytem && theytem && mytem->name != theytem->name) {
		SDL_Log("Different teams, doing the thing!");
		manager->remove(other);
		new hasFlag(manager, ent, theytem->name);

	} else if (!mytem || !theytem) {
		SDL_Log("Missing team component somewhere!");
	}
}
