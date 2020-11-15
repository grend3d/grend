#include <grend/gameState.hpp>

// TODO: move timing stuff to its own file, maybe have a profiler class...
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <glm/gtx/rotate_vector.hpp>

using namespace grendx;

// TODO: probably won't be used, convenience here is pretty minimal
// TODO: move to gameModel stuff
modelMap load_library(std::string dir) {
	modelMap ret;
	struct dirent *dirent;
	DIR *dirp;

	if (!(dirp = opendir(dir.c_str()))) {
		std::cerr << "Warning: couldn't load models from " << dir << std::endl;
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
			std::cerr << "   - " << fname << std::endl;;
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

static modelMap gen_internal_models(void) {
	return {
		{"unit_cube",        generate_cuboid(1, 1, 1)},
	};
}

// TODO: should start thinking about splitting initialization into smaller functions
gameState::gameState() {
	rootnode = gameObject::ptr(new gameObject());
	physObjects = gameObject::ptr(new gameObject());
}

gameState::~gameState() {
}
