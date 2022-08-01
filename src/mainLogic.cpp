// TODO: this file is oooooold, only function it still serves is providing
//       constructors/destructors for gameState, gameState itself is probably
//       not worth keeping around
#include <grend/gameState.hpp>
#include <grend/geometryGeneration.hpp>

// TODO: move timing stuff to its own file, maybe have a profiler class...
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <glm/gtx/rotate_vector.hpp>

using namespace grendx;

// TODO: probably won't be used, convenience here is pretty minimal
// TODO: move to sceneModel stuff
modelMap load_library(std::string dir) {
	modelMap ret;
	struct dirent *dirent;
	DIR *dirp;

	if (!(dirp = opendir(dir.c_str()))) {
		return ret;
	}

	while ((dirent = readdir(dirp))) {
		std::string fname = dirent->d_name;
		std::size_t pos = fname.rfind('.');

		if (pos == std::string::npos) {
			// no extension
			continue;
		}

		if (fname.substr(pos) == ".obj") {
			//ret[fname] = model(dir + "/" + fname);
			ret[fname] = load_object(dir + "/" + fname);
		}
	}

	closedir(dirp);
	return ret;
}

static std::pair<std::string, std::string> obj_models[] = {
	{"person",                  "assets/obj/low-poly-character-rpg/boy.obj"},
	{"smoothsphere",            "assets/obj/smoothsphere.obj"},
	{"X-Axis-Pointer",          "assets/obj/UI/X-Axis-Pointer.obj"},
	{"Y-Axis-Pointer",          "assets/obj/UI/Y-Axis-Pointer.obj"},
	{"Z-Axis-Pointer",          "assets/obj/UI/Z-Axis-Pointer.obj"},
	{"X-Axis-Rotation-Spinner", "assets/obj/UI/X-Axis-Rotation-Spinner.obj"},
	{"Y-Axis-Rotation-Spinner", "assets/obj/UI/Y-Axis-Rotation-Spinner.obj"},
	{"Z-Axis-Rotation-Spinner", "assets/obj/UI/Z-Axis-Rotation-Spinner.obj"},
};

// TODO: should start thinking about splitting initialization into smaller functions
gameState::gameState() {
	rootnode = sceneNode::ptr(new sceneNode());
	physObjects = sceneNode::ptr(new sceneNode());
}

gameState::~gameState() {
}
