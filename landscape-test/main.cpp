#include <grend/gameMainDevWindow.hpp>
#include <grend/gameMainWindow.hpp>
#include <grend/gameObject.hpp>
#include <grend/playerView.hpp>
#include <grend/geometryGeneration.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <memory>
#include <chrono>

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
		//load_gltf_scene(GR_PREFIX "assets/obj/Mossberg-lowres/shotgun.gltf");

	// leaving old stuff here for debugging
	objs->transform.scale = glm::vec3(0.1f);
	objs->transform.position = glm::vec3(0, -1, 0);
	//objs->transform.scale = glm::vec3(2.f);
	//objs->transform.position = glm::vec3(-0.3, 1.1, 1.25);
	//objs->transform.rotation = glm::quat(glm::vec3(0, 3.f*M_PI/2.f, 0));

	compileModels(models);
	bindCookedMeshes();
	setNode("weapon", player->cameraObj, objs);

	// TODO: need better way to do this
	if (auto animations = findAnimations(objs)) {
		for (auto& [name, ptr] : animations->groups) {
			if (auto group = ptr.lock()) {
				group->weight = (name == "walking")? 1.0 : 0.0;
			}
		}
	}
}

static float thing(float x, float y) {
	return sin(x) + sin(y);
}

static glm::vec2 randomGradient(glm::vec2 i) {
	/*
	float vx = 3141592623918.0*i.x + 31415926.0 + 1234558198*i.y;
	float vy = 314159261231.0*i.y + 3141592.0 + 9876598237*i.x;
	return { cos(vx), sin(vy) };
	*/
	float random = 2920.f
		* sin(i.x * 21942.f + i.y*171324.f + 8912.f)
		* cos(i.x * 23157.f * i.y*217832.f + 9758.f);
	return { cos(random), sin(random) };
}

static float dotGradient(glm::vec2 i, glm::vec2 pos) {
	glm::vec2 gradient = randomGradient(i);
	glm::vec2 dist = pos - i;

	return glm::dot(dist, gradient);
}

static float perlinNoise(float x, float y) {
	glm::vec2 pos = { x, y };
	glm::vec2 grid[2] = {
		glm::floor(pos),
		glm::floor(pos) + glm::vec2(1.0)
	};
	glm::vec2 weight = pos - grid[0];
	float n[4];

	for (unsigned i = 0; i < 4; i++) {
		n[i] = dotGradient(glm::vec2(grid[!!(i&1)].x, grid[!!(i&2)].y), pos);
	}

	float ix0 = glm::mix(n[0], n[1], weight.x);
	float ix1 = glm::mix(n[2], n[3], weight.x);

	return max(0.0, glm::mix(ix0, ix1, weight.y));
}

static float landscapeThing(float x, float y) {
	auto scalednoise = [](float scale, float x, float y) {
		return scale*perlinNoise(x / scale, y / scale);
	};

	float a1 = max(0.f, scalednoise(50.f, x, y));
	float a2 = scalednoise(10.f, x, y);
	float a3 = scalednoise(3.f,  x, y);
	float a4 = scalednoise(0.5f, x, y);

	return a1 + a2 + a3 + a4;
}

material landscapeMaterial = {
	.diffuse = {1, 1, 1, 1},
	.ambient = {1, 1, 1, 1},
	.specular = {1, 1, 1, 1},
	.emissive = {1, 1, 1, 1},
	.roughness = 1.f,
	.metalness = 0.f,
	.opacity = 1,
	.refract_idx = 1.5,

	.diffuseMap          = materialTexture("/home/flux/blender/tex/aerial_grass_rock_2k_jpg/aerial_grass_rock_diff_2k.jpg"),
	.metalRoughnessMap   = materialTexture("/home/flux/blender/tex/aerial_grass_rock_2k_jpg/aerial_grass_rock_rough_green_2k.jpg"),
	.normalMap           = materialTexture("/home/flux/blender/tex/aerial_grass_rock_2k_jpg/aerial_grass_rock_nor_2k.jpg"),
	.ambientOcclusionMap = materialTexture("/home/flux/blender/tex/aerial_grass_rock_2k_jpg/aerial_grass_rock_ao_2k.jpg"),
	.emissiveMap         = materialTexture(GR_PREFIX "assets/tex/black.png"),
};

static gameObject::ptr landscapeNodes = std::make_shared<gameObject>();

class landscapeGenView : public playerView {
	public:
		landscapeGenView(gameMain *game) : playerView(game) {};
		virtual void logic(gameMain *game, float delta);
		std::future<bool> genjob;
};

static const int   gridsize = 7;
static const float cellsize = 24.f;

static gameObject::ptr
generateLandscape(glm::vec3 curpos, glm::vec3 genpos, gameMain *game) {
	static gameModel::ptr models[gridsize][gridsize];
	static gameModel::ptr temp[gridsize][gridsize];

	gameObject::ptr ret = std::make_shared<gameObject>();
	std::list<std::future<bool>> futures;

	glm::vec3 diff = curpos - genpos;
	std::cerr << "curpos != genpos, diff: "
		<< "(" << diff.z << "," << diff.y << "," << diff.z << ")"
		<< std::endl;

	for (int x = 0; x < gridsize; x++) {
		for (int y = 0; y < gridsize; y++) {
			int ax = x + diff.x;
			int ay = y + diff.z;

			if (models[x][y] == nullptr
			    || ax >= gridsize || ax < 0
			    || ay >= gridsize || ay < 0)
			{
				futures.push_back(game->jobs->addAsync([=] {
					float off = cellsize * (gridsize / 2);
					glm::vec3 coord = (curpos * cellsize) - glm::vec3(off, 0, off) + glm::vec3(x*cellsize, 0, y*cellsize);
					auto ptr = generateHeightmap(cellsize, cellsize, 4.0, coord.x, coord.z, landscapeThing);
					//auto ptr = generateHeightmap(24, 24, 0.5, coord.x, coord.z, thing);
					ptr->transform.position = glm::vec3(coord.x, 0, coord.z);
					gameMesh::ptr mesh =
						std::dynamic_pointer_cast<gameMesh>(ptr->nodes["mesh"]);
					mesh->material = "grass";
					ptr->materials["grass"] = landscapeMaterial;
					std::string name = "gen["+std::to_string(int(x))+"]["+std::to_string(int(y))+"]" + std::to_string(rand());

					gameObject::ptr foo = std::make_shared<gameObject>();
					setNode("asdfasdf", foo, ptr);
					std::cerr << "trying to add physics model" << std::endl;
					game->phys->addStaticModels(foo, TRS());

					auto fut = game->jobs->addDeferred([=]{
							std::cerr << "generating new model" << std::endl;
						compileModel(name, ptr);
						return true;
					});

					fut.wait();

					temp[x][y] = ptr;
					return true;
				}));

			} else {
				temp[x][y] = models[ax][ay];
			}
		}
	}

	for (auto& fut : futures) {
		fut.wait();
	}

	auto meh = game->jobs->addDeferred([&] {
		for (int x = 0; x < gridsize; x++) {
			for (int y = 0; y < gridsize; y++) {
				models[x][y] = temp[x][y];
				temp[x][y] = nullptr;
				std::string name = "gen["+std::to_string(int(x))+"]["+std::to_string(int(y))+"]";
				setNode(name, ret, models[x][y]);
			}
		}

		return true;
	});
	meh.wait();

	auto fut = game->jobs->addDeferred([&] {
		bindCookedMeshes();
		return true;
	});

	fut.wait();
	return ret;
}

static glm::vec3 genpos = glm::vec3(INT_MAX);
void landscapeGenView::logic(gameMain *game, float delta) {
	static glm::vec3 lastvel = glm::vec3(0);
	static gameObject::ptr retval;

	if (cam->velocity() != lastvel) {
		cameraPhys->setAcceleration(cam->velocity());
		lastvel = cam->velocity();
	}

	game->phys->stepSimulation(1.f/game->frame_timer.last());
	cam->setPosition(cameraObj->transform.position - 5.f*cam->direction());

	glm::vec3 vel = cameraPhys->getVelocity();
	cameraObj->transform.rotation =
		glm::quat(glm::vec3(0, atan2(vel.x, vel.z), 0));

	glm::vec3 curpos = glm::floor((glm::vec3(1, 0, 1)*cameraObj->transform.position)/25.f);

	if (genjob.valid() && genjob.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
		genjob.get();
		setNode("nodes", landscapeNodes, retval);
		retval = nullptr;
	}

	if (!genjob.valid() && curpos != genpos) {
		std::cerr << "BBBBBBBBBB: starting new generator job" << std::endl;
		glm::vec3 npos = genpos;
		genpos = curpos;

		genjob = game->jobs->addAsync([=] {
			retval = generateLandscape(curpos, npos, game);
			return true;
		});
	}
}

int main(int argc, char *argv[]) {
	std::cerr << "entering main()" << std::endl;
	std::cerr << "started SDL context" << std::endl;
	std::cerr << "have game state" << std::endl;

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

	game->state->rootnode = loadMap(game);
	game->jobs->addAsync([=] {
		static gameObject::ptr retval;
		setNode("landscape", game->state->rootnode, landscapeNodes);
		genpos = glm::vec3(0);

		auto fut = game->jobs->addAsync([=] {
			retval = generateLandscape(glm::vec3(0), glm::vec3(INT_MAX), game);
			return true;
		});

		fut.wait();
		auto bindfut = game->jobs->addDeferred([=] {
			setNode("nodes", landscapeNodes, retval);
			return true;
		});

		bindfut.wait();
		return true;
	});

	game->phys->addStaticModels(game->state->rootnode, staticPosition);
	gameView::ptr player = std::make_shared<landscapeGenView>(game);
	game->setView(player);
	addCameraWeapon(player);

	game->run();
	return 0;
}
