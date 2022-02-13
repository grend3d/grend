#include <grend/gameObject.hpp>
#include <grend/utility.hpp>
#include <math.h>

using namespace grendx;

gameObject::~gameObject() {
	std::cerr << "Freeing a " << idString() << std::endl;
}

// XXX: "key functions", needed to do dynamic_cast across .so boundries
//      Requires that the function be a "non-inline, non-pure virtual function"
gameImport::~gameImport() {};
gameSkin::~gameSkin() {};
gameParticles::~gameParticles() {};
gameBillboardParticles::~gameBillboardParticles() {};
gameLight::~gameLight() {};
gameLightPoint::~gameLightPoint() {};
gameLightSpot::~gameLightSpot() {};
gameLightDirectional::~gameLightDirectional() {};
gameReflectionProbe::~gameReflectionProbe() {};
gameIrradianceProbe::~gameIrradianceProbe() {};

size_t grendx::allocateObjID(void) {
	static size_t counter = 0;
	return ++counter;
}

TRS gameObject::getTransformTRS(float delta) {
	if (animations.size() > 0) {
		TRS temp = transform;
		applyChannelVecTransforms(animations, temp, delta);
		return temp;

	} else {
		return transform;
	}
}

glm::mat4 gameObject::getTransformMatrix(float delta) {
	if (updated || animations.size() > 0) {
		TRS temp = getTransformTRS(delta);
		cachedTransformMatrix = temp.getTransform();
		updated = false;
		// note that queueCache.updated isn't changed here
	}

	return cachedTransformMatrix;
}

void gameObject::setTransform(const TRS& t) {
	updated = true;
	queueCache.updated = true;
	isDefault = false;
	transform = t;
}


void gameObject::setPosition(const glm::vec3& position) {
	TRS temp = getTransformTRS();
	temp.position = position;
	setTransform(temp);
}

void gameObject::setScale(const glm::vec3& scale) {
	TRS temp = getTransformTRS();
	temp.scale = scale;
	setTransform(temp);
}

void gameObject::setRotation(const glm::quat& rotation) {
	TRS temp = getTransformTRS();
	temp.rotation = rotation;
	setTransform(temp);
}

gameObject::ptr gameObject::getNode(std::string name) {
	return hasNode(name)? nodes[name] : nullptr;
}

gameObject::ptr grendx::unlink(gameObject::ptr node) {
	if (node != nullptr) {
		if (auto p = node->parent.lock()) {
			for (auto& [key, ptr] : p->nodes) {
				if (node == ptr) {
					gameObject::ptr ret = p;
					p->nodes.erase(key);
					node->parent.reset();
					return ret;
				}
			}
		}
	}

	return node;
}

gameObject::ptr grendx::clone(gameObject::ptr node) {
	gameObject::ptr ret = std::make_shared<gameObject>();

	ret->setTransform(node->getTransformTRS());

	for (auto& [name, ptr] : node->nodes) {
		setNode(name, ret, ptr);
	}

	return ret;
}

gameObject::ptr grendx::duplicate(gameObject::ptr node) {
	// TODO: need to copy all attributes

	gameObject::ptr ret;

	// copy specific per-type object members
	switch (node->type) {
		case gameObject::objType::None:
			ret = std::make_shared<gameObject>();
			break;

		case gameObject::objType::Import:
			{
				gameImport::ptr foo = std::static_pointer_cast<gameImport>(node);
				ret = std::make_shared<gameImport>(foo->sourceFile);
				break;
			}

		// TODO: meshes/models, particles shouldn't be copied, but would it make
		//       sense to copy other types like lights, cameras?
		default:
			return node;
	}

	// copy generic gameObject members
	ret->setTransform(node->getTransformTRS());
	ret->visible         = node->visible;
	ret->animations      = node->animations;
	ret->parent          = node->parent;
	ret->extraProperties = node->extraProperties;

	// copy sub-nodes
	for (auto& [name, ptr] : node->nodes) {
		setNode(name, ret, duplicate(ptr));
	}

	return ret;
}

std::string grendx::getNodeName(gameObject::ptr node) {
	if (node != nullptr) {
		if (auto p = node->parent.lock()) {
			for (auto& [key, ptr] : p->nodes) {
				if (node == ptr) {
					return key;
				}
			}
		}
	}

	return "";
}

void gameObject::removeNode(std::string name) {
	auto it = nodes.find(name);

	if (it != nodes.end()) {
		nodes.erase(it);
	}
}

bool gameObject::hasNode(std::string name) {
	return nodes.find(name) != nodes.end();
}

float gameLightPoint::extent(float threshold) {
	return radius * (sqrt((intensity * diffuse.w)/threshold) - 1);
}

float gameLightSpot::extent(float threshold) {
	return radius * (sqrt((intensity * diffuse.w)/threshold) - 1);
}

float gameLightDirectional::extent(float threshold) {
	// infinite extent
	return HUGE_VALF;
}

static glm::mat4 lookup(std::map<gameObject*, glm::mat4>& cache,
                        float tim,
                        gameObject *root,
                        gameObject *ptr)
{
	auto it = cache.find(ptr);

	if (it == cache.end()) {
		gameObject::ptr parent = ptr->parent.lock();
		glm::mat4 mat(1);

		if (ptr != root && parent) {
			mat = lookup(cache, tim, root, parent.get()) * ptr->getTransformMatrix(tim);
		}

		cache[ptr] = mat;
		return mat;

	} else {
		return it->second;
	}
}

void gameSkin::sync(Program::ptr program) { 
	size_t numjoints = min(inverseBind.size(), 1024);

	if (transforms.size() != inverseBind.size()) {
		transforms.resize(inverseBind.size());
	}

#if GLSL_VERSION >= 300
	// use UBOs on gles3, core profiles
	if (!ubuffer) {
		ubuffer = genBuffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
		ubuffer->allocate(sizeof(GLfloat[16*numjoints]));
	}
#endif

	float tim = SDL_GetTicks()/1000.f;
	std::map<gameObject*, glm::mat4> accumTransforms;

	for (unsigned i = 0; i < inverseBind.size(); i++) {
		gameObject::ptr temp = joints[i];
		glm::mat4 accum = lookup(accumTransforms, tim, this, joints[i].get());

		transforms[i] = accum*inverseBind[i];
	}

#if GLSL_VERSION < 300
	// no UBOs on gles2
	for (unsigned i = 0; i < transforms.size(); i++) {
		std::string sloc = "joints["+std::to_string(i)+"]";
		if (!prog->set(sloc, transforms[i])) {
			std::cerr <<
				"NOTE: couldn't set joint matrix " << i
				<< ", too many joints/wrong shader?" << std::endl;
			break;
		}
	}
#else
	ubuffer->update(transforms.data(), 0, sizeof(GLfloat[16*numjoints]));
	program->setUniformBlock("jointTransforms", ubuffer, UBO_JOINTS);
#endif
}

void gameParticles::syncBuffer(void) {
	if (!ubuffer) {
		ubuffer = genBuffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
		ubuffer->allocate(sizeof(GLfloat[16*maxInstances]));
	}

	if (!synced) {
		ubuffer->update(positions.data(), 0, sizeof(GLfloat[16*activeInstances]));
		synced = true;
	}
}

void gameParticles::update(void) {
	// just set a flag indicating that the buffer isn't synced,
	// will get synced in the render loop somewhere
	// (need to do it this way since things will be updated in threads, and
	// can't do anything to opengl state from non-main threads)
	synced = false;
}

gameParticles::gameParticles(unsigned _maxInstances)
	: gameObject(objType::Particles)
{
	positions.reserve(_maxInstances);
	positions.resize(positions.capacity());

	maxInstances = _maxInstances;
	activeInstances = 0;
};

void gameBillboardParticles::syncBuffer(void) {
	if (!ubuffer) {
		ubuffer = genBuffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
		ubuffer->allocate(sizeof(GLfloat[4*maxInstances]));
	}

	if (!synced) {
		ubuffer->update(positions.data(), 0, sizeof(GLfloat[4*activeInstances]));
		synced = true;
	}
}

void gameBillboardParticles::update(void) {
	// just set a flag indicating that the buffer isn't synced,
	// will get synced in the render loop somewhere
	// (need to do it this way since things will be updated in threads, and
	// can't do anything to opengl state from non-main threads)
	synced = false;
}

gameBillboardParticles::gameBillboardParticles(unsigned _maxInstances)
	: gameObject(objType::BillboardParticles)
{
	positions.reserve(_maxInstances);
	positions.resize(positions.capacity());

	maxInstances = _maxInstances;
	activeInstances = 0;
};
