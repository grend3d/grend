#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/link.hpp>

#include <grend-config.h>
#include <grend/glmIncludes.hpp>
#include <grend/animation.hpp>
// TODO: move, for lights
#include <grend/quadtree.hpp>
#include <grend/boundingBox.hpp>
#include <grend/TRS.hpp>

#include <memory>
#include <map>
#include <vector>
#include <utility>
#include <string>
#include <stdint.h>

#include <iostream>
#include <sstream>

namespace grendx {

size_t allocateObjID(void);

class sceneNode : public ecs::entity {
	public:
		// used for type checking, dynamically-typed tree here
		enum objType {
			None,
			Import,             // Imported model, indicates where to stop saving
			                    // + where to load from
			Model,              // generic model, static/dynamic/etc subclasses
			Mesh,               // meshes that make up a model
			Skin,               // Skinning info
			Particles,          // particle system
			BillboardParticles, // particle system
			Light,              // Light object, has Point/Spot/etc subclasses
			ReflectionProbe,    // Full reflection probe
			IrradianceProbe,    // diffuse light probe
			Camera,             // TODO: camera position marker
		} type = objType::None;

		typedef ecs::ref<sceneNode> ptr;
		typedef ecs::ref<sceneNode> weakptr;

		//sceneNode(enum objType t = objType::None) : type(t) {};
		sceneNode(ecs::regArgs t, enum objType newtype = objType::None)
			: ecs::entity(ecs::doRegister(this, t)),
			  type(newtype) {};
		virtual ~sceneNode();

		virtual std::string idString(void) {
			std::stringstream strm;
			strm << "[" << typeString() << " : 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		TRS getTransformTRS();
		TRS getOrigTransform();
		glm::mat4 getTransformMatrix();
		bool hasDefaultTransform(void) const { return isDefault; }

		void setTransform(const TRS& t);
		void setPosition(const glm::vec3& position);
		void setScale(const glm::vec3& scale);
		void setRotation(const glm::quat& rotation);

		// setNode isn't a member function, since it needs to be able to set
		// the shared pointer parent
		//sceneNode::ptr getNode(std::string name);
		// TODO: these are much less efficient now that that nodes aren't indexed in a map
		sceneNode::ptr getNode(std::string name);
		void removeNode(std::string name);
		bool hasNode(std::string name);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		// used for routing click events
		//size_t id = allocateObjID();
		// TODO: bounding box/radius

		// hash index into animation channel table
		uint32_t animChannel = 0;

		// whether the node visible
		bool visible = true;

		auto nodes() {
			return getAll<ecs::link<sceneNode>>();
		}

		sceneNode::weakptr parent;
		//std::map<std::string, sceneNode::ptr> nodes;
		// intended to map to gltf extra properties, useful for exporting
		// info from blender
		std::map<std::string, float> extraProperties;

		// cache for renderQueue state, queueCache only changed externally
		// from renderQueue::add(), queueUpdated changed both from 
		// setTransfrom() and renderQueue::add()
		// TODO: documented, link up documentation
		struct {
			bool      updated = true;
			glm::mat4 transform;
			glm::vec3 center;
		} queueCache;

	private:
		// TODO: transform setters/getters, cache in entity
		//TRS transform;
		TRS origTransform;

		bool updated = true;
		bool isDefault = true;
		glm::mat4 cachedTransformMatrix;
};

/*
static inline
void setNode(std::string name, sceneNode::ptr obj, sceneNode::ptr sub) {
	assert(obj != nullptr && sub != nullptr);

	//obj->nodes[name] = sub;
	//sub->parent = obj;
}

static inline
void setNodeXXX(std::string name, sceneNode::ptr obj, sceneNode::ptr sub) {
	assert(obj != nullptr && sub != nullptr);

	//obj->nodes[name] = sub;
}
*/

template <typename A, typename B>
void setNode(std::string name, A a, B b) {
	b->name = name;
	a->template attach<ecs::link<sceneNode>>(b.getPtr());
	b->parent = a;
}

static inline
glm::mat4 fullTranslation(sceneNode::ptr node) {
	if (node) {
		if (auto p = node->parent) {
			return fullTranslation(p) * node->getTransformMatrix();

		} else {
			return node->getTransformMatrix();
		}
	}

	return glm::mat4();
}

// TODO: just realized these overload syscalls, should that be avoided?
//       in principle it's fine, might be confusing in code though
sceneNode::ptr unlink(sceneNode::ptr node);
sceneNode::ptr clone(sceneNode::ptr node);     // shallow copy
sceneNode::ptr duplicate(sceneNode::ptr node); // deep copy
std::string getNodeName(sceneNode::ptr node);

class sceneImport : public sceneNode {
	public:
		typedef ecs::ref<sceneImport> ptr;
		typedef ecs::ref<sceneImport> weakptr;

		sceneImport(ecs::regArgs t, std::string_view path)
			: sceneNode(ecs::doRegister(this, t), objType::Import),
			  sourceFile(path)
		{
			// don't serialize links to imported entities
			disable(entity::serializeLinks);
		};

		sceneImport(ecs::regArgs t)
			: sceneNode(ecs::doRegister(this, t), objType::Import),
			  sourceFile("") {};

		virtual ~sceneImport();

		virtual std::string idString(void) {
			std::stringstream strm;
			strm << "[" << typeString() << " : " << sourceFile <<  "]";
			return strm.str();
		}

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		std::string sourceFile;
		animationCollection::ptr animations;
};

// forward declaration defined in glManager.hpp
// ugh, this is becoming a maze of forward declarations...
class Buffer;
class Program;

// TODO: does this need to be an entity? probably should just be a component
class sceneSkin : public sceneNode {
	public:
		typedef ecs::ref<sceneSkin> ptr;
		typedef ecs::ref<sceneSkin> weakptr;

		sceneSkin(ecs::regArgs t) : sceneNode(ecs::doRegister(this, t), objType::Skin) {}
		virtual ~sceneSkin();

		void sync(std::shared_ptr<Program> prog);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		std::vector<glm::mat4> inverseBind;
		std::vector<glm::mat4> transforms;
		// keep internal pointers to joints, same nodes as in the tree
		//std::vector<sceneNode::ptr> joints;
		// TODO: don't store pointers, store entity IDs
		std::vector<sceneNode::ptr> joints;

		std::shared_ptr<Buffer> ubuffer = nullptr;
};

class sceneParticles : public sceneNode {
	public:
		typedef ecs::ref<sceneParticles> ptr;
		typedef ecs::ref<sceneParticles> weakptr;

		sceneParticles(ecs::regArgs t, unsigned _maxInstances = 256);
		virtual ~sceneParticles();

		void update(void);
		void syncBuffer(void);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		std::vector<glm::mat4> positions;
		// approximate bounding sphere for instances in this object,
		// used for culling
		float radius;
		bool synced = false;
		unsigned activeInstances;
		unsigned maxInstances;

		std::shared_ptr<Buffer> ubuffer = nullptr;
};

class sceneBillboardParticles : public sceneNode {
	public:
		typedef ecs::ref<sceneBillboardParticles> ptr;
		typedef ecs::ref<sceneBillboardParticles> weakptr;

		sceneBillboardParticles(ecs::regArgs t, unsigned _maxInstances = 1024);
		virtual ~sceneBillboardParticles();

		void update(void);
		void syncBuffer(void);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		std::vector<glm::vec4> positions; /* xyz position, w scale */

		// approximate bounding sphere for instances in this object,
		// used for culling
		float radius;
		bool synced = false;
		unsigned activeInstances;
		unsigned maxInstances;

		std::shared_ptr<Buffer> ubuffer = nullptr;
};

class sceneLight : public sceneNode {
	public:
		typedef ecs::ref<sceneLight> ptr;
		typedef ecs::ref<sceneLight> weakptr;

		enum lightTypes {
			None,
			Point,
			Spot,
			Directional,
			Rectangular,  // TODO:
			Area,         // TODO:
			BoundedPoint, // TODO:
		} lightType;

		sceneLight(ecs::regArgs t)
			: sceneNode(ecs::doRegister(this, t), objType::Light),
			  lightType(None) {};

		sceneLight(ecs::regArgs t, enum lightTypes lighttype)
			: sceneNode(ecs::doRegister(this, t), objType::Light),
			  lightType(lighttype) {};
		virtual ~sceneLight();

		// XXX: need stub extent() here so this class can be instantiated, so that it can be
		//      added to the entity factories, so that it can be serialized and deserialized...
		//      should make a distinction between entities that can have their properties
		//      serialized and entities that can be (directly) instantiated
		//virtual float extent(float threshold=0.03) = 0;
		virtual float extent(float threshold=0.03) { return 0.0; };
		// TODO: 'within(threshold, pos)' to test if a light affects a given point

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		// TODO: why not store things in std140 format to avoid packing?
		glm::vec4 diffuse = glm::vec4(1);
		float     intensity = 50.0;
		bool      casts_shadows = false;
		bool changed   = true;
		bool is_static = true;
		bool have_map  = false;
};

class sceneLightPoint : public sceneLight {
	public:
		typedef ecs::ref<sceneLightPoint> ptr;
		typedef ecs::ref<sceneLightPoint> weakptr;

		sceneLightPoint(ecs::regArgs t)
			: sceneLight(ecs::doRegister(this, t), lightTypes::Point) {};
		virtual ~sceneLightPoint();

		virtual float extent(float threshold=0.03);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		float radius = 1.0f;
		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap[6];
};

class sceneLightSpot : public sceneLight {
	public:
		typedef ecs::ref<sceneLightSpot> ptr;
		typedef ecs::ref<sceneLightSpot> weakptr;

		sceneLightSpot(ecs::regArgs t)
			: sceneLight(ecs::doRegister(this, t), lightTypes::Spot) {};
		virtual ~sceneLightSpot();

		virtual float extent(float threshold=0.03);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		float radius = 1.0;
		float angle = 3.1415/4.0;

		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap;
		glm::mat4 shadowproj;
};

class sceneLightDirectional : public sceneLight {
	public:
		typedef ecs::ref<sceneLightDirectional> ptr;
		typedef ecs::ref<sceneLightDirectional> weakptr;

		sceneLightDirectional(ecs::regArgs t)
			: sceneLight(ecs::doRegister(this, t), lightTypes::Directional) {};
		virtual ~sceneLightDirectional();

		virtual float extent(float threshold=0.03);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap;
		glm::mat4 shadowproj;
};

class sceneReflectionProbe : public sceneNode {
	public:
		typedef ecs::ref<sceneReflectionProbe> ptr;
		typedef ecs::ref<sceneReflectionProbe> weakptr;

		sceneReflectionProbe(ecs::regArgs t)
			: sceneNode(ecs::doRegister(this, t), objType::ReflectionProbe) {};
		virtual ~sceneReflectionProbe();

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		quadtree::node_id faces[5][6];
		// bounding box for parallax correction
		AABB boundingBox = {
			.min = glm::vec3(-1),
			.max = glm::vec3(1),
		};

		bool changed = true;
		bool is_static = true;
		bool have_map = false;
		bool have_convolved = false;
};

class sceneIrradianceProbe : public sceneNode {
	public:
		typedef ecs::ref<sceneIrradianceProbe> ptr;
		typedef ecs::ref<sceneIrradianceProbe> weakptr;

		sceneIrradianceProbe(ecs::regArgs t)
			: sceneNode(ecs::doRegister(this, t), objType::IrradianceProbe)
		{
			source = t.manager->construct<sceneReflectionProbe>();
		};
		virtual ~sceneIrradianceProbe();

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		// TODO: not this
		sceneReflectionProbe *source;

		quadtree::node_id faces[6];
		quadtree::node_id coefficients[6];

		AABB boundingBox = {
			.min = glm::vec3(-1),
			.max = glm::vec3(1),
		};

		bool changed = true;
		bool is_static = true;
		bool have_map = false;
};

// namespace grendx
}
