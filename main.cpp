#include <grend/gameMainDevWindow.hpp>
#include <grend/gameMainWindow.hpp>
#include <grend/gameObject.hpp>
#include <grend/playerView.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <iostream>
#include <memory>
#include <chrono>

// XXX:  toggle using textures/models I have locally, don't want to
//       bloat the assets folder again
#define LOCAL_BUILD 1

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

	float a1 = max(0.f, scalednoise(75.f, x, y));
	float a2 = scalednoise(20.f, x, y);
	float a3 = scalednoise(5.f,  x, y);
	float a4 = 0.5*scalednoise(1.f, x, y);

	return a1 + a2 + a3 + a4;
}

material::ptr landscapeMaterial;
static gameObject::ptr landscapeNodes = std::make_shared<gameObject>();
static gameModel::ptr treeNode;

class worldGenerator {
	public:
		virtual gameObject::ptr getNode(void) { return root; };
		virtual void setPosition(gameMain *game, glm::vec3 position) = 0;

	protected:
		gameObject::ptr root = std::make_shared<gameObject>();
		glm::vec3 lastPosition = glm::vec3(INT_MAX);
};

class landscapeGenerator : public worldGenerator {
	public:
		landscapeGenerator(unsigned seed = 0xcafebabe);
		virtual void setPosition(gameMain *game, glm::vec3 position);

	private:
		void generateLandscape(gameMain *game, glm::vec3 curpos, glm::vec3 lastpos);
		std::future<bool> genjob;
		gameObject::ptr returnValue;
};

static const int   gridsize = 9;
static const float cellsize = 24.f;

landscapeGenerator::landscapeGenerator(unsigned seed) {
	// TODO: do something with the seed
}

void landscapeGenerator::generateLandscape(gameMain *game,
                                           glm::vec3 curpos,
                                           glm::vec3 lastpos)
{
	static gameModel::ptr models[gridsize][gridsize];
	static gameModel::ptr temp[gridsize][gridsize];

	gameObject::ptr ret = std::make_shared<gameObject>();
	std::list<std::future<bool>> futures;

	glm::vec3 diff = curpos - lastpos;
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
					auto ptr = generateHeightmap(cellsize, cellsize, 1.0, coord.x, coord.z, landscapeThing);
					//auto ptr = generateHeightmap(24, 24, 0.5, coord.x, coord.z, thing);
					ptr->transform.position = glm::vec3(coord.x, 0, coord.z);
					gameMesh::ptr mesh =
						std::dynamic_pointer_cast<gameMesh>(ptr->nodes["mesh"]);
					mesh->meshMaterial = landscapeMaterial;
					std::string name = "gen["+std::to_string(int(x))+"]["+std::to_string(int(y))+"]";

					gameObject::ptr foo = std::make_shared<gameObject>();
					setNode("asdfasdf", foo, ptr);
					std::cerr << "trying to add physics model" << std::endl;

					auto fut = game->jobs->addDeferred([=]{
							std::cerr << "generating new model" << std::endl;
						compileModel(name, ptr);
						//bindModel(ptr->comped_model);
						return true;
					});

					glm::vec2 posgrad = randomGradient(glm::vec2(coord.x, coord.z));
					float baseElevation = landscapeThing(coord.x, coord.z);
					int randtrees = (posgrad.x + 1.0)*0.5 * 5 * (1.0 - baseElevation/50.0);

					game->phys->addStaticModels(foo, TRS());

					for (int i = 0; i < randtrees; i++) {
						gameObject::ptr ntree = std::make_shared<gameObject>();
						glm::vec2 pos = randomGradient(glm::vec2(coord.x + i, coord.z + i));

						float tx = ((pos.x + 1)*0.5) * cellsize;
						float ty = ((pos.y + 1)*0.5) * cellsize;

						ntree->transform.position = glm::vec3(
							tx, landscapeThing(coord.x + tx, coord.z + ty) - 0.1, ty
						);
						ntree->transform.scale = glm::vec3((posgrad.y + 1.0)*0.5*3.0+0.5);

						std::string name = "tree["+std::to_string(i)+"]";
						setNode("tree", ntree, treeNode);
						setNode(name, ptr, ntree);
					}

					int randlight = (posgrad.y + 1.0)*0.5 * 7 * (1.0 - baseElevation/50.0);

					for (int i = 0; i < randlight; i++) {
						gameLightPoint::ptr nlit = std::make_shared<gameLightPoint>();
						glm::vec2 pos = randomGradient(glm::vec2(2*coord.x + i, 2*coord.z + i));

						float tx = ((pos.x + 1)*0.5) * cellsize;
						float ty = ((pos.y + 1)*0.5) * cellsize;

						nlit->radius = 0.15;
						nlit->intensity = 500.0;
						nlit->transform.position = glm::vec3(
							tx, landscapeThing(coord.x + tx, coord.z + ty) + 1.5, ty
						);

						std::string name = "point["+std::to_string(i)+"]";
						setNode(name, ptr, nlit);
					}

					temp[x][y] = ptr;
					fut.wait();
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
	returnValue = ret;
}

void landscapeGenerator::setPosition(gameMain *game, glm::vec3 position) {
	glm::vec3 curpos = glm::floor((glm::vec3(1, 0, 1)*position)/cellsize);

	if (genjob.valid() && genjob.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
		genjob.get();
		setNode("nodes", root, returnValue);
		returnValue = nullptr;
	}

	if (!genjob.valid() && curpos != lastPosition) {
		std::cerr << "BBBBBBBBBB: starting new generator job" << std::endl;
		glm::vec3 npos = lastPosition;
		lastPosition = curpos;

		genjob = game->jobs->addAsync([=] {
			generateLandscape(game, curpos, npos);
			return true;
		});
	}
}

class landscapeGenView : public playerView {
	public:
		typedef std::shared_ptr<landscapeGenView> ptr;
		typedef std::weak_ptr<landscapeGenView>   weakptr;

		landscapeGenView(gameMain *game) : playerView(game) {
#if LOCAL_BUILD
			enemyModel = loadScene("test-assets/obj/test-enemy.glb");
#else
			gameModel::ptr mod = generate_cuboid(1.0, 2.0, 0.5);
			enemyModel = std::make_shared<gameObject>();
			compileModel("emeny", mod);
			setNode("model", enemyModel, mod);
#endif

			for (unsigned i = 0; i < 15; i++) {
				gameObject::ptr temp = std::make_shared<gameObject>();

				temp->transform.position = glm::vec3(
					float(rand()) / RAND_MAX * 100.0 - 50,
					50.0,
					float(rand()) / RAND_MAX * 100.0 - 50
				);

				std::string name = "enemy["+std::to_string(i)+"]";
				game->phys->addSphere(temp, temp->transform.position, 1.0, 1.0);
				temp->physObj->setAngularFactor(0.0);
				enemies.push_back(temp);

				setNode("model", temp, enemyModel);
				setNode(name, enemyNodes, temp);
			}
		};

		virtual void logic(gameMain *game, float delta);
		gameObject::ptr getEnemies(void) { return enemyNodes; };

		std::vector<gameObject::ptr> enemies;
		gameObject::ptr enemyNodes = std::make_shared<gameObject>();
		gameObject::ptr enemyModel;
		landscapeGenerator landscape;
};

void landscapeGenView::logic(gameMain *game, float delta) {
	static glm::vec3 lastvel = glm::vec3(0);
	static gameObject::ptr retval;

	if (cam->velocity() != lastvel) {
		cameraPhys->setVelocity(cam->velocity());
		lastvel = cam->velocity();
	}

	game->phys->stepSimulation(1.f/game->frame_timer.last());
	cam->setPosition(cameraObj->transform.position - 5.f*cam->direction());

	for (auto& it : enemies) {
		glm::vec3 diff = cameraObj->transform.position - it->transform.position;
		glm::vec3 vel =  glm::normalize(glm::vec3(diff.x, 0, diff.z));
		it->physObj->setAcceleration(10.f*vel);
	}

	glm::vec3 vel = cameraPhys->getVelocity();
	cameraObj->transform.rotation =
		glm::quat(glm::vec3(0, atan2(vel.x, vel.z), 0));

	landscape.setPosition(game, cameraObj->transform.position);
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
	game->phys->addStaticModels(game->state->rootnode, staticPosition);

	landscapeGenView::ptr player = std::make_shared<landscapeGenView>(game);
	player->landscape.setPosition(game, glm::vec3(1));
	player->cam->setFar(1000.0);
	game->setView(player);
	setNode("enemies", game->state->rootnode, player->getEnemies());
	setNode("landscape", game->state->rootnode, player->landscape.getNode());
	addCameraWeapon(player);

	game->run();
	return 0;
}
