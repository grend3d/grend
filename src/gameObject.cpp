#include <grend/sceneNode.hpp>
#include <grend/glManager.hpp>
#include <grend/utility.hpp>
#include <grend/logger.hpp>
#include <math.h>

#include <grend/ecs/ecs.hpp>

using namespace grendx;

sceneNode::~sceneNode() {}

// XXX: "key functions", needed to do dynamic_cast across .so boundries
//      Requires that the function be a "non-inline, non-pure virtual function"
sceneImport::~sceneImport() {};
sceneSkin::~sceneSkin() {};
sceneParticles::~sceneParticles() {};
sceneBillboardParticles::~sceneBillboardParticles() {};
sceneLight::~sceneLight() {};
sceneLightPoint::~sceneLightPoint() {};
sceneLightSpot::~sceneLightSpot() {};
sceneLightDirectional::~sceneLightDirectional() {};
sceneReflectionProbe::~sceneReflectionProbe() {};
sceneIrradianceProbe::~sceneIrradianceProbe() {};

size_t grendx::allocateObjID(void) {
	static size_t counter = 0;
	return ++counter;
}

// TODO: should have this return a const ref
TRS sceneNode::getTransformTRS() {
	return transform;
}

// TODO: should have this return a const ref
TRS sceneNode::getOrigTransform() {
	return origTransform;
}

// TODO: also const ref
glm::mat4 sceneNode::getTransformMatrix() {
	if (updated) {
		cachedTransformMatrix = transform.getTransform();
		updated = false;
		// note that queueCache.updated isn't changed here
	}

	return cachedTransformMatrix;
}

void sceneNode::setTransform(const TRS& t) {
	if (isDefault) {
		origTransform = t;
	}

	updated = true;
	queueCache.updated = true;
	isDefault = false;
	transform = t;
}

void sceneNode::setPosition(const glm::vec3& position) {
	TRS temp = getTransformTRS();
	temp.position = position;
	setTransform(temp);
}

void sceneNode::setScale(const glm::vec3& scale) {
	TRS temp = getTransformTRS();
	temp.scale = scale;
	setTransform(temp);
}

void sceneNode::setRotation(const glm::quat& rotation) {
	TRS temp = getTransformTRS();
	temp.rotation = rotation;
	setTransform(temp);
}

sceneNode::ptr sceneNode::getNode(std::string name) {
	for (auto link : nodes()) {
		auto node = link->getRef();
		if (node->name == name) {
			return node;
		}
	}

	return nullptr;
}

sceneNode::ptr grendx::unlink(sceneNode::ptr node) {
	auto manager = engine::Resolve<ecs::entityManager>();

	if (auto p = node->parent) {
		for (auto ptr : p->nodes()) {
			if (node == ptr->getRef()) {
				sceneNode::ptr ret = p;
				// TODO: convenience function in entity for this
				// TODO: actual implementation
				manager->unregisterComponent(p.getPtr(), node.getPtr());
				return ret;
			}
		}
	}

	return node;
}

// TODO: rewrite
[[deprecated("needs to be rewritten")]]
sceneNode::ptr grendx::clone(sceneNode::ptr node) {
	auto manager = engine::Resolve<ecs::entityManager>();
	sceneNode::ptr ret = manager->construct<sceneNode>();
	//sceneNode::ptr ret = std::make_shared<sceneNode>();

	ret->setTransform(node->getTransformTRS());

	//for (auto& [name, ptr] : node->nodes) {
	//for (auto ptr : node->nodes()) {
		//auto& name = ptr->name;
		//setNode(name, ret, ptr);
	//}

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
	for (size_t i = 0; i < vec.size(); i++) {
		if (vec[i] == value) {
			return i;
		}
	}

	return -1;
}

static sceneNode::ptr copySkinNodes(sceneSkin::ptr target,
                                     sceneSkin::ptr skin,
                                     sceneNode::ptr node)
{
	if (!node) {
		return node;
	}

	if (node->type != sceneNode::objType::None) {
		// should only have pure node types under skin nodes
		return node;
	}

	auto manager = engine::Resolve<ecs::entityManager>();
	//sceneNode::ptr ret = std::make_shared<sceneNode>();
	sceneNode::ptr ret = manager->construct<sceneNode>();

	// copy generic sceneNode members
	// XXX: two set transforms to ensure origTransform is the same
	ret->setTransform(node->getOrigTransform());
	ret->setTransform(node->getTransformTRS());
	ret->visible         = node->visible;
	ret->animChannel     = node->animChannel;
	ret->extraProperties = node->extraProperties;

	// TODO: bounds check
	int i = indexOf(skin->joints, node);
	if (i >= 0) {
		target->joints[i] = ret;
	}

	// copy sub-nodes
	for (auto link : node->nodes()) {
		auto ptr = link->getRef();
		auto& name = ptr->name;
		setNode(name, ret, copySkinNodes(target, skin, ptr));
	}

	return ret;
}

static void copySkin(sceneSkin::ptr target, sceneSkin::ptr skin) {
	target->inverseBind = skin->inverseBind;
	target->transforms  = skin->transforms;

	target->joints.resize(skin->joints.size());

	for (auto link : skin->nodes()) {
		auto ptr = link->getRef();
		auto& name = ptr->name;
		setNode(name, target, copySkinNodes(target, skin, ptr));
	}
}

sceneNode::ptr grendx::duplicate(sceneNode::ptr node) {
	// TODO: need to copy all attributes

	auto manager = engine::Resolve<ecs::entityManager>();

	auto doCopy = [&] (sceneNode::ptr ret, sceneNode::ptr node) {
		// copy generic sceneNode members
		// XXX: two set transforms to ensure origTransform is the same
		ret->setTransform(node->getOrigTransform());
		ret->setTransform(node->getTransformTRS());
		ret->visible         = node->visible;
		ret->animChannel     = node->animChannel;
		ret->extraProperties = node->extraProperties;

		// copy sub-nodes
		for (auto link : node->nodes()) {
			auto ptr = link->getRef();
			auto sub = duplicate(ptr);

			sub->name = ptr->name;
			ret->attach<ecs::link<sceneNode>>(sub.getPtr());

			//auto& name = ptr->name;
			//setNode(name, ret, duplicate(ptr));
		}

		return ret;
	};

	// copy specific per-type object members
	switch (node->type) {
		case sceneNode::objType::None: {
				sceneNode::ptr ret = manager->construct<sceneNode>();
				return doCopy(ret, node);
		   }

		case sceneNode::objType::Import:
			{
				auto foo = node->get<sceneImport>();
				auto temp = manager->construct<sceneImport>(foo->sourceFile);
				temp->animations = copyCollection(foo->animations);
				return doCopy(temp, node);
			}

		case sceneNode::objType::Skin:
			{
				//ret = std::make_shared<sceneSkin>();
				sceneNode::ptr ret = manager->construct<sceneSkin>();

				// small XXX: need to copy skin information after copying subnodes
				//auto skin = std::static_pointer_cast<sceneSkin>(node);
				//auto temp = std::static_pointer_cast<sceneSkin>(ret);
				auto skin = node->get<sceneSkin>();
				auto temp = manager->construct<sceneSkin>();

				copySkin(temp, skin);
				return ret;
			}
			break;

		// TODO: meshes/models, particles shouldn't be copied, but would it make
		//       sense to copy other types like lights, cameras?
		default:
			return node;
	}
}

std::string grendx::getNodeName(sceneNode::ptr node) {
	return node? node->name : "";
}

void sceneNode::removeNode(std::string name) {
	// XXX
	auto ent = getNode(name);
	// TODO:
	// TODO: convenience function
	//manager->unregisterComponent(this, ent.getPtr());

	/*
	auto it = nodes.find(name);

	if (it != nodes.end()) {
		nodes.erase(it);
	}
	*/
}

bool sceneNode::hasNode(std::string name) {
	// XXX: inefficient
	for (auto ptr : nodes()) {
		if ((*ptr)->name == name) {
			return true;
		}
	}

	return false;
}

float sceneLightPoint::extent(float threshold) {
	return radius * (sqrt((intensity * diffuse.w)/threshold) - 1);
}

float sceneLightSpot::extent(float threshold) {
	return radius * (sqrt((intensity * diffuse.w)/threshold) - 1);
}

float sceneLightDirectional::extent(float threshold) {
	// infinite extent
	return HUGE_VALF;
}

// TODO: better name
static glm::mat4 lookup(std::map<sceneNode*, glm::mat4>& cache,
                        sceneNode *root,
                        sceneNode *ptr)
{
	auto it = cache.find(ptr);

	if (it == cache.end()) {
		sceneNode::ptr parent = ptr->parent;
		glm::mat4 mat(1);

		if (ptr != root && parent) {
			mat = lookup(cache, root, parent.getPtr()) * ptr->getTransformMatrix();
		}

		cache[ptr] = mat;
		return mat;

	} else {
		return it->second;
	}
}

void sceneSkin::sync(Program::ptr program) { 
	size_t numjoints = min(inverseBind.size(), 256ul);

	if (transforms.size() != inverseBind.size()) {
		transforms.resize(inverseBind.size());
	}

#if GLSL_VERSION >= 300
	// use UBOs on gles3, core profiles
	if (!ubuffer) {
		ubuffer = genBuffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
		//ubuffer->allocate(sizeof(GLfloat[16*numjoints]));
		ubuffer->allocate(sizeof(GLfloat[16*256]));
	}
#endif

	std::map<sceneNode*, glm::mat4> accumTransforms;

	for (unsigned i = 0; i < inverseBind.size(); i++) {
		if (!joints[i]) {
			transforms[i] = glm::mat4(1);
			continue;
		}

		glm::mat4 accum = lookup(accumTransforms, this, joints[i].getPtr());
		transforms[i] = accum*inverseBind[i];
	}

#if GLSL_VERSION < 300
	// no UBOs on gles2
	for (unsigned i = 0; i < transforms.size(); i++) {
		std::string sloc = "joints["+std::to_string(i)+"]";
		if (!prog->set(sloc, transforms[i])) {
			LogWarnFmt("NOTE: couldn't set joint matrix "
			           ", too many joints/wrong shader?", i);
			break;
		}
	}
#else
	ubuffer->update(transforms.data(), 0, sizeof(GLfloat[16*numjoints]));
	program->setUniformBlock("jointTransforms", ubuffer, UBO_JOINTS);
#endif
}

void sceneParticles::syncBuffer(void) {
	if (!ubuffer) {
		ubuffer = genBuffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
		ubuffer->allocate(sizeof(GLfloat[16*256]));
	}

	if (!synced) {
		ubuffer->update(positions.data(), 0, sizeof(GLfloat[16*activeInstances]));
		synced = true;
	}
}

void sceneParticles::update(void) {
	// just set a flag indicating that the buffer isn't synced,
	// will get synced in the render loop somewhere
	// (need to do it this way since things will be updated in threads, and
	// can't do anything to opengl state from non-main threads)
	synced = false;
}

sceneParticles::sceneParticles(ecs::regArgs t, unsigned _maxInstances)
	: sceneNode(ecs::doRegister(this, t), objType::Particles)
{
	positions.reserve(_maxInstances);
	positions.resize(positions.capacity());

	maxInstances = _maxInstances;
	activeInstances = 0;
};

void sceneBillboardParticles::syncBuffer(void) {
	if (!ubuffer) {
		ubuffer = genBuffer(GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
		ubuffer->allocate(sizeof(GLfloat[4*1024]));
	}

	if (!synced) {
		ubuffer->update(positions.data(), 0, sizeof(GLfloat[4*activeInstances]));
		synced = true;
	}
}

void sceneBillboardParticles::update(void) {
	// just set a flag indicating that the buffer isn't synced,
	// will get synced in the render loop somewhere
	// (need to do it this way since things will be updated in threads, and
	// can't do anything to opengl state from non-main threads)
	synced = false;
}

sceneBillboardParticles::sceneBillboardParticles(ecs::regArgs t, unsigned _maxInstances)
	: sceneNode(ecs::doRegister(this, t), objType::BillboardParticles)
{
	positions.reserve(_maxInstances);
	positions.resize(positions.capacity());

	maxInstances = _maxInstances;
	activeInstances = 0;
};

nlohmann::json sceneNode::serializer(ecs::component *comp) {
	return {};
}

nlohmann::json sceneImport::serializer(ecs::component *comp) {
	sceneImport *ent = static_cast<sceneImport*>(comp);

	return {
		{"sourceFile", ent->sourceFile},
	};
}

nlohmann::json sceneSkin::serializer(ecs::component *comp) {
	return {};
}

nlohmann::json sceneParticles::serializer(ecs::component *comp) {
	return {};
}

nlohmann::json sceneBillboardParticles::serializer(ecs::component *comp) {
	return {};
}

nlohmann::json sceneLight::serializer(ecs::component *comp) {
	auto *light = static_cast<sceneLight*>(comp);
	auto& d = light->diffuse;

	return {
		{"diffuse",       {d[0], d[1], d[2], d[3]}},
		{"intensity",     light->intensity},
		{"casts_shadows", light->casts_shadows},
		{"is_static",     light->is_static},
	};

	return {};
}

nlohmann::json sceneLightPoint::serializer(ecs::component *comp) {
	auto *light = static_cast<sceneLightPoint*>(comp);

	return {
		{"radius", light->radius},
	};
}

nlohmann::json sceneLightSpot::serializer(ecs::component *comp) {
	auto *light = static_cast<sceneLightSpot*>(comp);

	return {
		{"radius", light->radius},
		{"angle",  light->angle},
	};
}

nlohmann::json sceneLightDirectional::serializer(ecs::component *comp) {
	return {};
}

nlohmann::json sceneReflectionProbe::serializer(ecs::component *comp) {
	auto probe = static_cast<sceneReflectionProbe*>(comp);
	auto bmin = probe->boundingBox.min;
	auto bmax = probe->boundingBox.max;

	return {
		{"boundingBox", {
			{"min", {bmin[0], bmin[1], bmin[2]}},
			{"max", {bmax[0], bmax[1], bmax[2]}},
		}},

		{"is_static", probe->is_static},
	};
}

nlohmann::json sceneIrradianceProbe::serializer(ecs::component *comp) {
	auto probe = static_cast<sceneIrradianceProbe*>(comp);
	auto bmin = probe->boundingBox.min;
	auto bmax = probe->boundingBox.max;

	return {
		{"boundingBox", {
			{"min", {bmin[0], bmin[1], bmin[2]}},
			{"max", {bmax[0], bmax[1], bmax[2]}},
		}},

		{"is_static", probe->is_static},
	};
}

void sceneNode::deserializer(ecs::component *comp, nlohmann::json j) {}

void sceneImport::deserializer(ecs::component *comp, nlohmann::json j) {
	auto im = static_cast<sceneImport*>(comp);

	im->sourceFile = j["sourceFile"];
}

void sceneSkin::deserializer(ecs::component *comp, nlohmann::json j) {}

void sceneParticles::deserializer(ecs::component *comp, nlohmann::json j) {}

void sceneBillboardParticles::deserializer(ecs::component *comp, nlohmann::json j) {}

void sceneLight::deserializer(ecs::component *comp, nlohmann::json j) {
	auto *light = static_cast<sceneLight*>(comp);
	auto& diff = j["diffuse"];

	light->diffuse       = glm::vec4(diff[0], diff[1], diff[2], diff[3]);
	light->intensity     = j["intensity"];
	light->casts_shadows = j["casts_shadows"];
	light->is_static     = j["is_static"];
}

void sceneLightPoint::deserializer(ecs::component *comp, nlohmann::json j) {
	auto *light = static_cast<sceneLightPoint*>(comp);
	light->radius = j["radius"];
}

void sceneLightSpot::deserializer(ecs::component *comp, nlohmann::json j) {
	auto *light = static_cast<sceneLightSpot*>(comp);

	light->radius = j["radius"];
	light->angle  = j["angle"];
}

void sceneLightDirectional::deserializer(ecs::component *comp, nlohmann::json j) {}

void sceneReflectionProbe::deserializer(ecs::component *comp, nlohmann::json j) {
	auto *probe = static_cast<sceneReflectionProbe*>(comp);

	auto bbox = j["boundingBox"];
	auto bmin = bbox["min"];
	auto bmax = bbox["max"];

	probe->is_static       = j["is_static"];
	probe->boundingBox.min = glm::vec3(bmin[0], bmin[1], bmin[2]);
	probe->boundingBox.max = glm::vec3(bmax[0], bmax[1], bmax[2]);
}

void sceneIrradianceProbe::deserializer(ecs::component *comp, nlohmann::json j) {
	auto *probe = static_cast<sceneIrradianceProbe*>(comp);

	auto bbox = j["boundingBox"];
	auto bmin = bbox["min"];
	auto bmax = bbox["max"];

	probe->is_static       = j["is_static"];
	probe->boundingBox.min = glm::vec3(bmin[0], bmin[1], bmin[2]);
	probe->boundingBox.max = glm::vec3(bmax[0], bmax[1], bmax[2]);
}
