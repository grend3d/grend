#include <grend/gameMainDevWindow.hpp>
#include <grend/gameObject.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <memory>

using namespace grendx;

// TODO: library functions to handle most of this
void addCameraWeapon(gameView::ptr view) {
	playerView::ptr player = std::dynamic_pointer_cast<playerView>(view);
	gameObject::ptr testweapon = std::make_shared<gameObject>();

	auto [objs, models] =
		load_gltf_scene(GR_PREFIX "assets/obj/Mossberg-lowres/shotgun.gltf");

	compile_models(models);
	bind_cooked_meshes();
	objs->scale = glm::vec3(2.0f);
	objs->position = glm::vec3(-0.3, 1.1, 1.25);
	objs->rotation = glm::quat(glm::vec3(0, 3.f*M_PI/2.f, 0));
	setNode("weapon", player->cameraObj, objs);
}

int main(int argc, char *argv[]) {
	std::cerr << "entering main()" << std::endl;
	std::cerr << "started SDL context" << std::endl;
	std::cerr << "have game state" << std::endl;

	gameMainDevWindow *game = new gameMainDevWindow();
	// TODO: shouldn't need access to gameMain subclass to add objects relative
	//       to the camera, need a current camera object or something
	addCameraWeapon(game->player);
	game->run();

	return 0;
}
