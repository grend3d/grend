#include <grend/gameObject.hpp>
#include <math.h>

using namespace grendx;

gameObject::~gameObject() {
	std::cerr << "Freeing a " << idString() << std::endl;

	if (physObj != nullptr) {
		physObj->removeSelf();
	}
}

// XXX: "key functions", needed to do dynamic_cast across .so boundries
//      Requires that the function be a "non-inline, non-pure virtual function"
gameImport::~gameImport() {};
gameSkin::~gameSkin() {};
gameParticles::~gameParticles() {};
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

glm::mat4 gameObject::getTransform(float delta) {
	struct TRS temp = transform;
	//animations.applyTransform(temp, delta);
	applyChannelVecTransforms(animations, temp, delta);
	return temp.getTransform();
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

	ret->transform = node->transform;

	for (auto& [name, ptr] : node->nodes) {
		setNode(name, ret, ptr);
	}

	return ret;
}

gameObject::ptr grendx::duplicate(gameObject::ptr node) {
	// TODO: need to copy all attributes
	/*
	gameObject::ptr ret = std::make_shared<gameObject>();

	for (auto& [name, ptr] : node->nodes) {
		setNode(name, ret, duplicate(ptr));
	}
	*/

	return clone(node);
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
