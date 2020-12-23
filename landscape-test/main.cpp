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
#include <map>
#include <vector>
#include <set>

#include <initializer_list>

// XXX:  toggle using textures/models I have locally, don't want to
//       bloat the assets folder again
#define LOCAL_BUILD 0

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

class animatedCharacter {
	public:
		typedef std::shared_ptr<animatedCharacter> ptr;
		typedef std::weak_ptr<animatedCharacter>   weakptr;

		animatedCharacter(gameObject::ptr objs);
		void setAnimation(std::string animation, float weight = 1.0);
		gameObject::ptr getObject(void);

	private:
		animationCollection::ptr animations;
		std::string currentAnimation;
		gameObject::ptr objects;
};

animatedCharacter::animatedCharacter(gameObject::ptr objs) {
	if ((animations = findAnimations(objs)) == nullptr) {
		// TODO: proper error
		throw "asdf";
	}

	objects = objs;
}

gameObject::ptr animatedCharacter::getObject(void) {
	return objects;
}

void animatedCharacter::setAnimation(std::string animation, float weight) {
	if (animations) {
		for (auto& [name, ptr] : animations->groups) {
			if (auto group = ptr.lock()) {
				group->weight = (name == animation)? weight : 0.0;
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

struct inputEvent {
	enum types {
		move,
		look,
		crouch,
		// left, right, middle click
		primaryAction,
		secondaryAction,
		tertiaryAction,
	} type;

	glm::vec3 data;
	bool active;
};

typedef std::shared_ptr<std::vector<inputEvent>> inputQueue;

class entitySystem;
class component;
class entity;

// TODO: any way to make the tags generalized for any iterable container type?
bool intersects(std::multimap<std::string, component*>& entdata,
                std::initializer_list<std::string> test)
{
	for (auto& s : test) {
		auto it = entdata.find(s);
		if (it == entdata.end()) {
			return false;
		}
	}

	return true;
}

bool intersects(std::multimap<std::string, component*>& entdata,
                std::vector<std::string>& test)
{
	for (auto& s : test) {
		auto it = entdata.find(s);
		if (it == entdata.end()) {
			return false;
		}
	}

	return true;
}

class entityManager {
	public:
		typedef std::shared_ptr<entityManager> ptr;
		typedef std::weak_ptr<entityManager>   weakptr;

		entityManager(gameMain *_engine) : engine(_engine) {};

		std::map<std::string, std::shared_ptr<entitySystem>> systems;
		std::map<std::string, std::set<component*>> components;
		std::map<entity*, std::multimap<std::string, component*>> entityComponents;
		std::map<component*, entity*> componentEntities;
		std::set<entity*> entities;

		void update(float delta);
		void registerComponent(entity *ent, std::string name, component *ptr);

		void add(entity *ent);
		void remove(entity *ent);
		bool hasComponents(entity *ent, std::initializer_list<std::string> tags);
		bool hasComponents(entity *ent, std::vector<std::string> tags);

		std::set<component*>& getComponents(std::string name);
		std::multimap<std::string, component*>& getEntityComponents(entity *ent);
		entity *getEntity(component *com); 

		gameObject::ptr root = std::make_shared<gameObject>();

		// TODO: should this be similar to inputHandlerSystem, with
		//       onCollision components?
		std::shared_ptr<std::vector<collision>> collisions
			= std::make_shared<std::vector<collision>>();

		// XXX
		gameMain *engine;
};

class component {
	public:
		component(entityManager *manager, entity *ent) {
			//manager->registerComponent(ent, "component", this);
		}

		// draw the imgui widgets for this component, TODO
		// also, this class needs a polymorphic member in order to upcast,
		// so this fills that role.
		virtual void drawEditorWidgets(void) { };
};

class entity : public component {
	public:
		typedef std::shared_ptr<entity> ptr;
		typedef std::weak_ptr<entity>   weakptr;

		entity(entityManager *manager)
			: component(manager, this)
		{
			manager->registerComponent(this, "entity", this);
		}

		virtual void update(entityManager *manager, float delta) = 0;
		virtual gameObject::ptr getNode(void) { return node; };

		gameObject::ptr node = std::make_shared<gameObject>();
};

class controllable : public component {
	public: 
		controllable(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "controllable", this);
		}

		void handleInput(inputEvent& ev);
};

// XXX: having basically tag components, waste of space?
class isControlled : public component {
	public: 
		isControlled(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "isControlled", this);
		}
};

class rigidBody : public component {
	public:
		rigidBody(entityManager *manager, entity *ent, float _mass)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "rigidBody", this);
			mass = _mass;
		}

		void registerCollisionQueue(std::shared_ptr<std::vector<collision>> queue) {
			if (phys) {
				phys->collisionQueue = queue;
			}
			// TODO: should show warning if there's no physics object
		}

		void syncPhysics(entity *ent) {
			if (ent && ent->node && phys) {
				ent->node->transform = phys->getTransform();
			}
		}

		physicsObject::ptr phys = nullptr;
		float mass;
};

class rigidBodySphere : public rigidBody {
	public:
		rigidBodySphere(entityManager *manager,
		                entity *ent,
		                glm::vec3 position,
		                float mass,
		                float radius)
			: rigidBody(manager, ent, mass)
		{
			manager->registerComponent(ent, "rigidBodySphere", this);
			phys = manager->engine->phys->addSphere(ent, position, mass, radius);
		}
};

class rigidBodyBox : public rigidBody {
	public:
		rigidBodyBox(entityManager *manager,
		             entity *ent,
		             glm::vec3 position,
		             float mass,
		             AABBExtent& box)
			: rigidBody(manager, ent, mass)
		{
			manager->registerComponent(ent, "rigidBodyBox", this);
			phys = manager->engine->phys->addBox(ent, position, mass, box);
		}
};

class collisionHandler : public component {
	public:
		collisionHandler(entityManager *manager, entity *ent,
		                 std::vector<std::string> _tags)
			: component(manager, ent),
			  tags(_tags)
		{
			manager->registerComponent(ent, "collisionHandler", this);
		}

		virtual void
		onCollision(entityManager *manager, entity *ent, collision& col) = 0;

		std::vector<std::string> tags;
};

class projectileCollision : public collisionHandler {
	public:
		projectileCollision(entityManager *manager, entity *ent)
			: collisionHandler(manager, ent, {"projectile"})
		{
			manager->registerComponent(ent, "projectileCollision", this);
		}

		virtual void
		onCollision(entityManager *manager, entity *ent, collision& col) {
			std::cerr << "projectile collision!" << std::endl;
			if (false /* check health or something */) {
				manager->remove(ent);
			}
		};
};

class projectileDestruct : public collisionHandler {
	public:
		projectileDestruct(entityManager *manager, entity *ent)
			: collisionHandler(manager, ent, {"projectileCollision"})
		{
			manager->registerComponent(ent, "projectileDestruct", this);
		}

		virtual void
		onCollision(entityManager *manager, entity *ent, collision& col) {
			std::cerr << "projectile destruct!" << std::endl;
			manager->remove(ent);
		};
};

class inputHandler : public component {
	public:
		inputHandler(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "inputHandler", this);
		}

		virtual void
		handleInput(entityManager *manager, entity *ent, inputEvent& ev) {
			std::cerr << "handleInput(): got here" << std::endl;
		}
};

template <class T>
T castEntityComponent(entityManager *m, entity *e, std::string name) {
	auto &comps = m->getEntityComponents(e);
	auto comp = comps.find(name);

	if (comp == comps.end()) {
		// TODO: error?
		return nullptr;
	}

	return dynamic_cast<T>(comp->second);
}

class movementHandler : public inputHandler {
	public:
		movementHandler(entityManager *manager, entity *ent)
			: inputHandler(manager, ent)
		{
			manager->registerComponent(ent, "movementHandler", this);
		}

		virtual void
		handleInput(entityManager *manager, entity *ent, inputEvent& ev) {
			if (ev.active && ev.type == inputEvent::types::move) {
				rigidBody *body =
					castEntityComponent<rigidBody*>(manager, ent, "rigidBody");

				if (body && ev.data != lastvel) {
					body->phys->setVelocity(ev.data);
					lastvel = ev.data;
				}
			}
		}

		glm::vec3 lastvel;
};

class entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual void update(entityManager *manager, float delta) {}
};

class entitySystemCollision : public entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual void update(entityManager *manager, float delta);
};

class inputHandlerSystem : public entitySystem {
	public:
		typedef std::shared_ptr<entitySystem> ptr;
		typedef std::weak_ptr<entitySystem>   weakptr;

		virtual void update(entityManager *manager, float delta);

		inputQueue inputs = std::make_shared<std::vector<inputEvent>>();
};

void entitySystemCollision::update(entityManager *manager, float delta) {
	for (auto& col : *manager->collisions) {
		if (col.adata && col.bdata) {
			entity *ents[2] = {
				static_cast<entity*>(col.adata),
				static_cast<entity*>(col.bdata),
			};

			for (unsigned i = 0; i < 2; i++) {
				entity *self  = ents[i];
				entity *other = ents[!i];

				if (manager->hasComponents(self, {"collisionHandler", "entity"})) {
					collisionHandler *collider =
						castEntityComponent<collisionHandler*>(manager, self, "collisionHandler");

					/*
					auto& comps = manager->getEntityComponents(other);
					std::cerr << "components: ";
					for (auto& [name, comp] : comps) {
						std::cerr << name << "(" << comp << ")" << ", ";
					}
					std::cerr << std::endl;
					*/

					if (!collider) {
						// no collision component...?
						continue;
					}

					if (manager->hasComponents(other, collider->tags)) {
						collider->onCollision(manager, self, col);
					}
				}
			}
		}
	}
}

void inputHandlerSystem::update(entityManager *manager, float delta) {
	auto handlers = manager->getComponents("inputHandler");

	for (auto& ev : *inputs) {
		for (auto& it : handlers) {
			inputHandler *handler = dynamic_cast<inputHandler*>(it);
			entity *ent = manager->getEntity(handler);

			if (handler && ent) {
				handler->handleInput(manager, ent, ev);
			}
		}
	}

	inputs->clear();
}

void entityManager::update(float delta) {
	for (auto& [name, system] : systems) {
		if (system) {
			system->update(this, delta);
		}
	}

	for (auto& ent : entities) {
		ent->update(this, delta);
	}

	collisions->clear();
}

void entityManager::add(entity *ent) {
	setNode("entity["+std::to_string((uintptr_t)ent)+"]", root, ent->getNode());
	entities.insert(ent);
}

void entityManager::remove(entity *ent) {
	if (!entities.count(ent)) {
		// not a valid entity
		return;
	}

	root->removeNode("entity["+std::to_string((uintptr_t)ent)+"]");

	auto comps = entityComponents[ent];

	for (auto& [name, comp] : comps) {
		componentEntities.erase(comp);
		components[name].erase(comp);
	}

	entityComponents.erase(ent);
	entities.erase(ent);
	//delete ent; // TODO: don't leak memory ok
}

// TODO: specialization or w/e
bool entityManager::hasComponents(entity *ent,
                                  std::initializer_list<std::string> tags)
{
	auto& compmap = entityComponents[ent];
	return intersects(compmap, tags);
}

bool entityManager::hasComponents(entity *ent,
                                  std::vector<std::string> tags)
{
	auto& compmap = entityComponents[ent];
	return intersects(compmap, tags);
}

std::set<entity*> searchEntities(entityManager *manager,
                                 std::initializer_list<std::string> tags)
{
	std::set<entity*> ret;
	std::vector<std::set<entity*>> candidates;

	/*
	// left here for debugging, just in case
	for (auto& ent : manager->entities) {
		if (manager->hasComponents(ent, tags)) {
			ret.insert(ent);
		}
	}
	*/

	if (tags.size() == 0) {
		// lol
		return {};

	} else if (tags.size() == 1) {
		auto& comps = manager->getComponents(*tags.begin());

		for (auto& comp : comps) {
			ret.insert(manager->getEntity(comp));
		}

	} else {
		// TODO: could this be done without generating the sets of entities...?
		//       would need a map of component names -> entities, which would be
		//       messy since an entity can have more than one of any component...
		//       this is probably good enough, just don't search for "entity" :P
		for (auto& tag : tags) {
			auto& comps = manager->getComponents(tag);
			candidates.push_back({});

			for (auto& comp : comps) {
				candidates.back().insert(manager->getEntity(comp));
			}
		}

		// find smallest (most exclusive) set of candidates
		auto smallest = candidates.begin();
		for (auto it = candidates.begin(); it != candidates.end(); it++) {
			if (it->size() < smallest->size()) {
				smallest = it;
			}
		}

		for (auto ent : *smallest) {
			bool inAll = true;

			for (auto it = candidates.begin(); it != candidates.end(); it++) {
				if (it == smallest)  continue;

				if (!it->count(ent)) {
					inAll = false;
					break;
				}
			}

			if (inAll) {
				ret.insert(ent);
			}
		}
	}

	return ret;
}

entity *findNearest(entityManager *manager,
                    glm::vec3 position,
                    std::initializer_list<std::string> tags)
{
	float curmin = HUGE_VALF;
	entity *ret = nullptr;

	auto ents = searchEntities(manager, tags);
	for (auto& ent : ents) {
		gameObject::ptr node = ent->getNode();
		float dist = glm::distance(position, node->transform.position);

		if (dist < curmin) {
			curmin = dist;
			ret = ent;
		}
	}

	return ret;
}

entity *findFirst(entityManager *manager,
                  std::initializer_list<std::string> tags)
{
	// TODO: search by component names
	for (auto& ent : manager->entities) {
		if (manager->hasComponents(ent, tags)) {
			return ent;
		}
	}

	return nullptr;
}


std::set<component*>& entityManager::getComponents(std::string name) {
	// XXX: avoid creating component names if nothing registered them
	static std::set<component*> nullret;
	//std::cerr << "looking up components for " << name << std::endl;

	return (components.count(name))? components[name] : nullret;
}

std::multimap<std::string, component*>&
entityManager::getEntityComponents(entity *ent) {
	// XXX: similarly, something which isn't registered here has 
	//      no components (at least, in this system) so return empty set
	static std::multimap<std::string, component*> nullret;

	return (entityComponents.count(ent))? entityComponents[ent] : nullret;
}

entity *entityManager::getEntity(component *com) {
	auto it = componentEntities.find(com);
	if (it != componentEntities.end()) {
		return it->second;

	} else {
		return nullptr;
	}
}

void entityManager::registerComponent(entity *ent,
                                      std::string name,
                                      component *ptr)
{
	// TODO: need a proper logger
	std::cerr << "registering component '" << name << "' for " << ptr << std::endl;
	components[name].insert(ptr);
	entityComponents[ent].insert({name, ptr});
	componentEntities.insert({ptr, ent});
}

class projectile : public entity {
	public:
		projectile(entityManager *manager, gameMain *game, glm::vec3 position);
		virtual void update(entityManager *manager, float delta);
};

void projectile::update(entityManager *manager, float delta) {
	rigidBody *body =
		castEntityComponent<rigidBody*>(manager, this, "rigidBody");

	if (body) {
		body->syncPhysics(this);
	}
}

class boxBullet : public projectile {
	public:
		boxBullet(entityManager *manager, gameMain *game, glm::vec3 position);
};

class boxSpawner : public inputHandler {
	public:
		boxSpawner(entityManager *manager, entity *ent)
			: inputHandler(manager, ent)
		{
			manager->registerComponent(ent, "boxSpawner", this);
		}

		virtual void
		handleInput(entityManager *manager, entity *ent, inputEvent& ev) {
			if (ev.active && ev.type == inputEvent::types::primaryAction) {
				std::cerr << "boxSpawner::handleInput(): got here" << std::endl;

				auto box = new boxBullet(manager, manager->engine, ent->node->transform.position + 3.f*ev.data);

				rigidBody *body =
					castEntityComponent<rigidBody*>(manager, box, "rigidBody");

				if (body) {
					body->phys->setVelocity(40.f * ev.data);
					manager->add(box);
				}
			}
		}
};

class enemy : public entity {
	public:
		enemy(entityManager *manager, gameMain *game, glm::vec3 position);
		virtual void update(entityManager *manager, float delta);
		virtual gameObject::ptr getNode(void) { return node; };

		std::shared_ptr<std::vector<collision>> collisions
			= std::make_shared<std::vector<collision>>();

		float health;
		rigidBody *body;
};

class player : public entity {
	public:
		player(entityManager *manager, gameMain *game, glm::vec3 position);
		virtual void update(entityManager *manager, float delta);
		virtual gameObject::ptr getNode(void) { return node; };

		std::shared_ptr<std::vector<collision>> collisions
			= std::make_shared<std::vector<collision>>();
		float health;

		animatedCharacter::ptr character;
		rigidBody *body;
};

projectile::projectile(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager)
{
	manager->registerComponent(this, "projectile", this);

	// TODO: configurable projectile attributes
	new rigidBodySphere(manager, this, position, 1.0, 0.15);
	new projectileDestruct(manager, this);
	node->transform.position = position;
}

boxBullet::boxBullet(entityManager *manager, gameMain *game, glm::vec3 position)
	: projectile(manager, game, position)
{
	static gameObject::ptr model = nullptr;

	manager->registerComponent(this, "boxBullet", this);

	if (!model) {
#if LOCAL_BUILD
		model = loadScene("test-assets/obj/smoothcube.glb");
#else
		gameModel::ptr mod = generate_cuboid(0.3, 0.3, 0.3);
		model = std::make_shared<gameObject>();
		compileModel("boxProjectile", mod);
		setNode("model", model, mod);
#endif
		bindCookedMeshes();
	}

	setNode("model", node, model);
}

enemy::enemy(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager),
	  body(new rigidBodySphere(manager, this, position, 1.0, 1.0))
{
	static gameObject::ptr enemyModel = nullptr;

	new projectileCollision(manager, this);
	manager->registerComponent(this, "enemy", this);

	// TODO:
	if (!enemyModel) {
#if LOCAL_BUILD
		enemyModel = loadScene("test-assets/obj/test-enemy.glb");
		enemyModel->transform.scale = glm::vec3(0.3);
#else
		gameModel::ptr mod = generate_cuboid(1.0, 2.0, 0.5);
		enemyModel = std::make_shared<gameObject>();
		compileModel("emeny", mod);
		setNode("model", enemyModel, mod);
#endif
	}

	node->transform.position = position;
	setNode("model", node, enemyModel);
	body->registerCollisionQueue(manager->collisions);
	body->phys->setAngularFactor(0.0);
}

void enemy::update(entityManager *manager, float delta) {
	glm::vec3 playerPos;

	entity *playerEnt =
		findNearest(manager, node->transform.position, {"player"});

	if (playerEnt) {
		playerPos = playerEnt->getNode()->transform.position;
	}

	glm::vec3 diff = playerPos - node->transform.position;
	glm::vec3 vel =  glm::normalize(glm::vec3(diff.x, 0, diff.z));
	body->phys->setAcceleration(10.f*vel);
	body->syncPhysics(this);

	collisions->clear();
}

player::player(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager),
	  body(new rigidBodySphere(manager, this, position, 1.0, 1.0))
{
	static gameObject::ptr playerModel = nullptr;

	new boxSpawner(manager, this);
	new movementHandler(manager, this);
	new projectileCollision(manager, this);

	manager->registerComponent(this, "player", this);

	if (!playerModel) {
		// TODO: resource cache
		playerModel = loadScene(GR_PREFIX "assets/obj/TestGuy/rigged-lowpolyguy.glb");
		playerModel->transform.scale = glm::vec3(0.1f);
		playerModel->transform.position = glm::vec3(0, -1, 0);
		bindCookedMeshes();
	}

	node->transform.position = position;
	setNode("model", node, playerModel);
	character = std::make_shared<animatedCharacter>(playerModel);
	character->setAnimation("idle");

	body->registerCollisionQueue(manager->collisions);
}

void player::update(entityManager *manager, float delta) {
	TRS physTransform = body->phys->getTransform();
	node->transform.position = physTransform.position;
	glm::vec3 vel = body->phys->getVelocity();
	node->transform.rotation =
		glm::quat(glm::vec3(0, atan2(vel.x, vel.z), 0));

	if (glm::length(vel) < 2.0) {
		character->setAnimation("idle");
	} else {
		character->setAnimation("walking");
	}

	collisions->clear();
}

bindFunc inputMapper(inputQueue q, camera::ptr cam) {
	return [=] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:
				case SDLK_s:
				case SDLK_a:
				case SDLK_d:
				case SDLK_q:
				case SDLK_e:
				case SDLK_SPACE:
					// XXX: just sync movement with camera, will want
					//      to change this
					q->push_back({
						.type = inputEvent::types::move,
						.data = cam->velocity(),
						.active = ev.type == SDL_KEYDOWN,
					});
					break;
			}
		}

		if (ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP) {
			// down = active (clicked), up = inactive
			bool active = ev.type == SDL_MOUSEBUTTONDOWN;

			switch (ev.button.button) {
				case SDL_BUTTON_LEFT:
					q->push_back({
						.type = inputEvent::types::primaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_BUTTON_RIGHT:
					q->push_back({
						.type = inputEvent::types::secondaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_BUTTON_MIDDLE:
					q->push_back({
						.type = inputEvent::types::tertiaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;
			}
		}

		return MODAL_NO_CHANGE;
	};
}


class landscapeGenView : public playerView {
	public:
		typedef std::shared_ptr<landscapeGenView> ptr;
		typedef std::weak_ptr<landscapeGenView>   weakptr;

		landscapeGenView(gameMain *game) : playerView(game) {
			manager = std::make_shared<entityManager>(game);

			// TODO: names are kinda pointless here
			manager->systems["collision"] =
				std::make_shared<entitySystemCollision>();

			auto inputSys = std::make_shared<inputHandlerSystem>();
			manager->systems["input"] = inputSys;
			input.bind(modes::Move, inputMapper(inputSys->inputs, cam));

			manager->add(new player(manager.get(), game, glm::vec3(-15, 50, 0)));
			player *playerEnt = new player(manager.get(), game, glm::vec3(0, 20, 0));
			manager->add(playerEnt);

			for (unsigned i = 0; i < 15; i++) {
				glm::vec3 position = glm::vec3(
					float(rand()) / RAND_MAX * 100.0 - 50,
					50.0,
					float(rand()) / RAND_MAX * 100.0 - 50
				);

				manager->add(new enemy(manager.get(), game, position));
			}

			bindCookedMeshes();
		};

		virtual void logic(gameMain *game, float delta);
		virtual void render(gameMain *game);
		//void loadPlayer(void);

		entityManager::ptr manager;
		landscapeGenerator landscape;
};

void landscapeGenView::logic(gameMain *game, float delta) {
	static glm::vec3 lastvel = glm::vec3(0);
	static gameObject::ptr retval;

	if (cam->velocity() != lastvel) {
		lastvel = cam->velocity();
	}

	game->phys->stepSimulation(1.f/game->frame_timer.last());
	game->phys->filterCollisions();;
	
	entity *playerEnt = findFirst(manager.get(), {"player", "entity"});

	if (playerEnt) {
		TRS& transform = playerEnt->getNode()->transform;
		cam->setPosition(transform.position - 5.f*cam->direction());
		landscape.setPosition(game, transform.position);
	}

	manager->update(1.f / game->frame_timer.last());
}

void landscapeGenView::render(gameMain *game) {
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);

	if (input.mode == modes::MainMenu) {
		renderWorld(game, cam);

		// TODO: need to set post size on resize event..
		//post->setSize(winsize_x, winsize_y);
		post->draw(game->rend->framebuffer);
		input.setMode(modes::Move);

		// TODO: function to do this
		//drawMainMenu(winsize_x, winsize_y);

	} else {
		renderWorld(game, cam);
		post->draw(game->rend->framebuffer);

		Framebuffer().bind();
		setDefaultGlFlags();

		disable(GL_DEPTH_TEST);
		disable(GL_SCISSOR_TEST);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		nvgBeginFrame(vgui.nvg, game->rend->screen_x, game->rend->screen_y, 1.0);
		nvgSave(vgui.nvg);

		/*
		for (unsigned i = 0; i < enemies.size(); i++) {
			auto& ptr = enemies[i];
			float ticks = SDL_GetTicks() / 1000.f;
			glm::vec4 pos = cam->worldToScreenPosition(ptr->transform.position + glm::vec3(0, 10, 0));

			if (cam->onScreen(pos)) {
				// TODO: some sort of grid editor wouldn't be too hard,
				//       probably worthwhile for quick UIs
				float depth = 16*max(0.f, pos.w);
				float pad = depth*16.f;

				float width  = 8*pad;
				float height = 6*pad;

				pos.y  = 1.0 - pos.y;
				pos.x *= game->rend->screen_x;
				pos.y *= game->rend->screen_y;

				glm::vec2 outer    = glm::vec2(pos) - glm::vec2(width, height)*0.5f;
				glm::vec2 innermin = outer + pad;
				glm::vec2 innermax = outer + glm::vec2(width, height) - 2*pad;

				nvgFontSize(vgui.nvg, pad);
				nvgFontFace(vgui.nvg, "sans-bold");
				nvgFontBlur(vgui.nvg, 0);
				nvgTextAlign(vgui.nvg, NVG_ALIGN_LEFT);

				nvgBeginPath(vgui.nvg);
				nvgRect(vgui.nvg, outer.x, outer.y, width, height);
				nvgFillColor(vgui.nvg, nvgRGBA(28, 30, 34, 192));
				nvgFill(vgui.nvg);

				float amount = sin(i*ticks)*0.5 + 0.5;
				nvgBeginPath(vgui.nvg);
				nvgRect(vgui.nvg, innermin.x, innermin.y, amount*(width - 2*pad), pad);
				nvgFillColor(vgui.nvg, nvgRGBA(0, 192, 0, 192));
				nvgFill(vgui.nvg);

				nvgBeginPath(vgui.nvg);
				nvgRect(vgui.nvg, innermin.x + amount*(width - 2*pad),
				        innermin.y, (1.f - amount)*(width - 2*pad), pad);
				nvgFillColor(vgui.nvg, nvgRGBA(192, 0, 0, 192));
				nvgFill(vgui.nvg);

				nvgFillColor(vgui.nvg, nvgRGBA(0xf0, 0x60, 0x60, 160));
				//nvgText(vgui.nvg, innermax.x, innermin.y + 1*pad, "❎", NULL);

				nvgFillColor(vgui.nvg, nvgRGBA(220, 220, 220, 160));
				nvgText(vgui.nvg, innermin.x, innermin.y + 2*pad, "💚 Enemy", NULL);
				nvgText(vgui.nvg, innermin.x, innermin.y + 3*pad, "❎ Some stats", NULL);
				nvgText(vgui.nvg, innermin.x, innermin.y + 4*pad, "❎ 123/456", NULL);
			}
		}
		*/

		nvgRestore(vgui.nvg);
		nvgEndFrame(vgui.nvg);
	}
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
	game->phys->addStaticModels(nullptr, game->state->rootnode, staticPosition);

	landscapeGenView::ptr player = std::make_shared<landscapeGenView>(game);
	player->landscape.setPosition(game, glm::vec3(1));
	player->cam->setFar(1000.0);
	game->setView(player);

	setNode("entities", game->state->rootnode, player->manager->root);
	setNode("landscape", game->state->rootnode, player->landscape.getNode());

	game->run();
	return 0;
}
