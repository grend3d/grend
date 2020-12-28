#include <grend/gameMain.hpp>
#include <grend/gameMainDevWindow.hpp>
#include <grend/gameObject.hpp>
#include <grend/playerView.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <memory>

using namespace grendx;

// TODO: only returns one collection, object tree could have any number
animationCollection::ptr findAnimations(gameObject::ptr obj) {
	if (!obj) {
		return nullptr;
	}

	for (auto& chan : obj->animations) {
		return chan->group->collection;
	}

	for (auto& [name, ptr] : obj->nodes) {
		auto ret = findAnimations(ptr);

		if (ret) {
			return ret;
		}
	}

	return nullptr;
}

// TODO: library functions to handle most of this
void addCameraWeapon(gameView::ptr view) {
	playerView::ptr player = std::dynamic_pointer_cast<playerView>(view);
	gameObject::ptr testweapon = std::make_shared<gameObject>();

	auto [objs, models] =
		load_gltf_scene(GR_PREFIX "assets/obj/TestGuy/rigged-lowpolyguy.glb");

	// leaving old stuff here for debugging
	objs->transform.scale = glm::vec3(0.1f);
	objs->transform.position = glm::vec3(0, -1, 0);

	compileModels(models);
	bindCookedMeshes();

	// TODO: need better way to do this
	if (auto animations = findAnimations(objs)) {
		for (auto& [name, ptr] : animations->groups) {
			if (auto group = ptr.lock()) {
				group->weight = (name == "walking")? 1.0 : 0.0;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: map-viewer filename.map" << std::endl;
		return 0;
	}

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

	game->state->rootnode = loadMap(game, argv[1]);
	game->phys->addStaticModels(nullptr, game->state->rootnode, staticPosition);
	gameView::ptr player = std::make_shared<playerView>(game);
	game->setView(player);
	addCameraWeapon(player);

	game->run();
	return 0;
}
