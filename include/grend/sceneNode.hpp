#pragma once

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

class sceneNode {
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

		typedef std::shared_ptr<sceneNode> ptr;
		typedef std::weak_ptr<sceneNode> weakptr;

		sceneNode(enum objType t = objType::None) : type(t) {};
		virtual ~sceneNode();

		virtual std::string typeString(void) {
			return "sceneNode";
		}

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
		sceneNode::ptr getNode(std::string name);
		void removeNode(std::string name);
		bool hasNode(std::string name);

		// used for routing click events
		size_t id = allocateObjID();
		// TODO: bounding box/radius

		// hash index into animation channel table
		uint32_t animChannel = 0;

		// whether the node visible
		bool visible = true;

		sceneNode::weakptr parent;
		std::map<std::string, sceneNode::ptr> nodes;
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
		TRS transform;
		TRS origTransform;

		bool updated = true;
		bool isDefault = true;
		glm::mat4 cachedTransformMatrix;
};

static inline
void setNode(std::string name, sceneNode::ptr obj, sceneNode::ptr sub) {
	assert(obj != nullptr && sub != nullptr);

	obj->nodes[name] = sub;
	sub->parent = obj;
}

static inline
void setNodeXXX(std::string name, sceneNode::ptr obj, sceneNode::ptr sub) {
	assert(obj != nullptr && sub != nullptr);

	obj->nodes[name] = sub;
}


// TODO: just realized these overload syscalls, should that be avoided?
//       in principle it's fine, might be confusing in code though
sceneNode::ptr unlink(sceneNode::ptr node);
sceneNode::ptr clone(sceneNode::ptr node);     // shallow copy
sceneNode::ptr duplicate(sceneNode::ptr node); // deep copy
std::string    getNodeName(sceneNode::ptr node);

class sceneImport : public sceneNode {
	public:
		typedef std::shared_ptr<sceneImport> ptr;
		typedef std::weak_ptr<sceneImport>   weakptr;

		sceneImport(std::string path)
			: sceneNode(objType::Import), sourceFile(path) {}
		virtual ~sceneImport();

		virtual std::string typeString(void) {
			return "Imported file";
		}

		virtual std::string idString(void) {
			std::stringstream strm;
			strm << "[" << typeString() << " : " << sourceFile <<  "]";
			return strm.str();
		}

		std::string sourceFile;
		animationCollection::ptr animations;
};

// forward declaration defined in glManager.hpp
// ugh, this is becoming a maze of forward declarations...
class Buffer;
class Program;

class sceneSkin : public sceneNode {
	public:
		typedef std::shared_ptr<sceneSkin> ptr;
		typedef std::weak_ptr<sceneSkin>   weakptr;

		sceneSkin() : sceneNode(objType::Skin) {}
		virtual ~sceneSkin();

		virtual std::string typeString(void) {
			return "Skin";
		}

		void sync(std::shared_ptr<Program> prog);

		std::vector<glm::mat4> inverseBind;
		std::vector<glm::mat4> transforms;
		// keep internal pointers to joints, same nodes as in the tree
		std::vector<sceneNode::ptr> joints;

		std::shared_ptr<Buffer> ubuffer = nullptr;
};

class sceneParticles : public sceneNode {
	public:
		typedef std::shared_ptr<sceneParticles> ptr;
		typedef std::weak_ptr<sceneParticles>   weakptr;

		sceneParticles(unsigned _maxInstances = 256);
		virtual ~sceneParticles();

		virtual std::string typeString(void) {
			return "Particle system";
		}

		void update(void);
		void syncBuffer(void);

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
		typedef std::shared_ptr<sceneBillboardParticles> ptr;
		typedef std::weak_ptr<sceneBillboardParticles>   weakptr;

		sceneBillboardParticles(unsigned _maxInstances = 1024);
		virtual ~sceneBillboardParticles();

		virtual std::string typeString(void) {
			return "Particle system";
		}

		void update(void);
		void syncBuffer(void);

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
		typedef std::shared_ptr<sceneLight> ptr;
		typedef std::weak_ptr<sceneLight> weakptr;

		enum lightTypes {
			None,
			Point,
			Spot,
			Directional,
			Rectangular,  // TODO:
			Area,         // TODO:
			BoundedPoint, // TODO:
		} lightType;

		sceneLight(enum lightTypes t)
			: sceneNode(objType::Light), lightType(t) {};
		virtual ~sceneLight();

		virtual std::string typeString(void) {
			return "Light (abstract)";
		}

		virtual float extent(float threshold=0.03) = 0;
		// TODO: 'within(threshold, pos)' to test if a light affects a given point

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
		typedef std::shared_ptr<sceneLightPoint> ptr;
		typedef std::weak_ptr<sceneLightPoint> weakptr;

		sceneLightPoint() : sceneLight(lightTypes::Point) {};
		virtual ~sceneLightPoint();

		virtual std::string typeString(void) {
			return "Point light";
		}

		virtual float extent(float threshold=0.03);

		float radius = 1.0f;
		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap[6];
};

class sceneLightSpot : public sceneLight {
	public:
		typedef std::shared_ptr<sceneLightSpot> ptr;
		typedef std::weak_ptr<sceneLightSpot> weakptr;

		sceneLightSpot() : sceneLight(lightTypes::Spot) {};
		virtual ~sceneLightSpot();

		virtual std::string typeString(void) {
			return "Spot light";
		}

		virtual float extent(float threshold=0.03);

		float radius = 1.0;
		float angle = 3.1415/4.0;

		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap;
		glm::mat4 shadowproj;
};

class sceneLightDirectional : public sceneLight {
	public:
		typedef std::shared_ptr<sceneLightDirectional> ptr;
		typedef std::weak_ptr<sceneLightDirectional> weakptr;

		sceneLightDirectional() : sceneLight(lightTypes::Directional) {};
		virtual ~sceneLightDirectional();

		virtual std::string typeString(void) {
			return "Directional light";
		}

		virtual float extent(float threshold=0.03);

		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap;
		glm::mat4 shadowproj;
};

class sceneReflectionProbe : public sceneNode {
	public:
		typedef std::shared_ptr<sceneReflectionProbe> ptr;
		typedef std::weak_ptr<sceneReflectionProbe> weakptr;

		virtual std::string typeString(void) {
			return "Reflection probe";
		}

		sceneReflectionProbe() : sceneNode(objType::ReflectionProbe) {};
		virtual ~sceneReflectionProbe();

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
		typedef std::shared_ptr<sceneIrradianceProbe> ptr;
		typedef std::weak_ptr<sceneIrradianceProbe> weakptr;

		virtual std::string typeString(void) {
			return "Irradiance probe";
		}

		sceneIrradianceProbe() : sceneNode(objType::IrradianceProbe) {
			source = std::make_shared<sceneReflectionProbe>();
		};
		virtual ~sceneIrradianceProbe();

		sceneReflectionProbe::ptr source;
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
