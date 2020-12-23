#include <grend/geometryGeneration.hpp>
#include <math.h>
#include "landscapeGenerator.hpp"

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

					game->phys->addStaticModels(nullptr, foo, TRS());
					gameParticles::ptr parts = std::make_shared<gameParticles>(32);
					parts->activeInstances = randtrees;
					parts->radius = cellsize / 2.f * 1.415;

					for (unsigned i = 0; i < parts->activeInstances; i++) {
						TRS transform;
						glm::vec2 pos = randomGradient(glm::vec2(coord.x + i, coord.z + i));

						float tx = ((pos.x + 1)*0.5) * cellsize;
						float ty = ((pos.y + 1)*0.5) * cellsize;

						transform.position = glm::vec3(
							tx, landscapeThing(coord.x + tx, coord.z + ty) - 0.1, ty
						);
						transform.scale = glm::vec3((posgrad.y + 1.0)*0.5*3.0+0.5);
						parts->positions[i] = transform.getTransform();
					}

					parts->update();
					setNode("tree", parts, treeNode);
					setNode("parts", ptr, parts);

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
