#include <grend/gameObject.hpp>
#include <grend/glManager.hpp>
#include <grend/utility.hpp>
#include <math.h>

using namespace grendx;

gameObject::~gameObject() {
	// TODO: toggleable debug log, or profile events, etc
	//std::cerr << "Freeing a " << idString() << std::endl;
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

// TODO: should have this return a const ref
TRS gameObject::getTransformTRS() {
	return transform;
}

// TODO: should have this return a const ref
TRS gameObject::getOrigTransform() {
	return origTransform;
}

// TODO: also const ref
glm::mat4 gameObject::getTransformMatrix() {
	if (updated) {
		cachedTransformMatrix = transform.getTransform();
		updated = false;
		// note that queueCache.updated isn't changed here
	}

	return cachedTransformMatrix;
}

void gameObject::setTransform(const TRS& t) {
	if (isDefault) {
		origTransform = t;
	}

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

static animationMap::ptr copyAnimationMap(animationMap::ptr anim) {
	auto ret = std::make_shared<animationMap>();

	for (auto& p : *anim) {
		ret->insert(p);
	}

	return ret;
}

static animationCollection::ptr copyCollection(animationCollection::ptr anims) {
	if (!anims) return nullptr;

	auto ret = std::make_shared<animationCollection>();

	for (auto& [name, anim] : *anims) {
		if (anim == nullptr)
			continue;

		ret->insert({name, copyAnimationMap(anim)});
	}

	return ret;
}

template <typename T>
static int indexOf(const std::vector<T>& vec, const T& value) {
	for (int i = 0; i < vec.size(); i++) {
		if (vec[i] == value) {
			return i;
		}
	}

	return -1;
}

static gameObject::ptr copySkinNodes(gameSkin::ptr target,
                                     gameSkin::ptr skin,
                                     gameObject::ptr node)
{
	if (node == nullptr) {
		return node;
	}

	if (node->type != gameObject::objType::None) {
		// should only have pure node types under skin nodes
		return node;
	}

	gameObject::ptr ret = std::make_shared<gameObject>();

	// copy generic gameObject members
	// XXX: two set transforms to ensure origTransform is the same
	ret->setTransform(node->getOrigTransform());
	ret->setTransform(node->getTransformTRS());
	ret->visible         = node->visible;
	ret->animChannel     = node->animChannel;
	ret->extraProperties = node->extraProperties;

	int i = indexOf(skin->joints, node);
	if (i >= 0) {
		target->joints[i] = ret;
	}

	// copy sub-nodes
	for (auto& [name, ptr] : node->nodes) {
		setNode(name, ret, copySkinNodes(target, skin, ptr));
	}

	return ret;
}

static void copySkin(gameSkin::ptr target, gameSkin::ptr skin) {
	target->inverseBind = skin->inverseBind;
	target->transforms  = skin->transforms;

	target->joints.resize(skin->joints.size());

	for (auto& [name, ptr] : skin->nodes) {
		setNode(name, target, copySkinNodes(target, skin, ptr));
	}
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
				auto foo = std::static_pointer_cast<gameImport>(node);
				auto temp = std::make_shared<gameImport>(foo->sourceFile);
				temp->animations = copyCollection(foo->animations);

				ret = temp;
				break;
			}

		case gameObject::objType::Skin:
			{
				ret = std::make_shared<gameSkin>();

				// small XXX: need to copy skin information after copying subnodes
				auto skin = std::static_pointer_cast<gameSkin>(node);
				auto temp = std::static_pointer_cast<gameSkin>(ret);

				copySkin(temp, skin);
				return ret;
			}
			break;

		// TODO: meshes/models, particles shouldn't be doCopy, but would it make
		//       sense to copy other types like lights, cameras?
		default:
			return node;
	}

	// copy generic gameObject members
	// XXX: two set transforms to ensure origTransform is the same
	ret->setTransform(node->getOrigTransform());
	ret->setTransform(node->getTransformTRS());
	ret->visible         = node->visible;
	ret->animChannel     = node->animChannel;
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

// TODO: better name
static glm::mat4 lookup(std::map<gameObject*, glm::mat4>& cache,
                        gameObject *root,
                        gameObject *ptr)
{
	auto it = cache.find(ptr);

	if (it == cache.end()) {
		gameObject::ptr parent = ptr->parent.lock();
		glm::mat4 mat(1);

		if (ptr != root && parent) {
			mat = lookup(cache, root, parent.get()) * ptr->getTransformMatrix();
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

	std::map<gameObject*, glm::mat4> accumTransforms;

	for (unsigned i = 0; i < inverseBind.size(); i++) {
		if (!joints[i]) {
			transforms[i] = glm::mat4(1);
			continue;
		}

		glm::mat4 accum = lookup(accumTransforms, this, joints[i].get());
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
