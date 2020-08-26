#pragma once
#include <grend/glm-includes.hpp>
#include <grend/opengl-includes.hpp>
#include <grend/sdl-context.hpp>
// TODO: move, for lights
#include <grend/quadtree.hpp>

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

// TODO: start switching things to camelCase
class gameObject {
	public:
		// used for type checking, dynamically-typed tree here
		enum objType {
			None,
			Model,           // generic model, static/dynamic/etc subclasses
			Mesh,            // meshes that make up a model
			Particles,       // particle system
			Light,           // Light object, has Point/Spot/etc subclasses
			ReflectionProbe, // Full reflection probe
			Lightprobe,      // diffuse light probe
			Camera,          // TODO: camera position marker
		} type = objType::None;

		typedef std::shared_ptr<gameObject> ptr;
		typedef std::weak_ptr<gameObject> weakptr;

		gameObject(enum objType t = objType::None) : type(t) {};

		// handlers for basic input events
		virtual void onLeftClick() {
			// default handlers just call upwards
			std::cerr << "left click " << typeString() << std::endl;
			if (parent) { parent->onLeftClick(); }
		};

		virtual void onMiddleClick() {
			std::cerr << "middle click " << typeString() << std::endl;
			if (parent) { parent->onMiddleClick(); }
		};

		virtual void onRightClick() {
			std::cerr << "right click " << typeString() << std::endl;
			if (parent) { parent->onRightClick(); }
		};

		virtual void onHover() {
			std::cerr << "hover " << typeString() << std::endl;
			if (parent) { parent->onHover(); }
		};

		virtual std::string typeString(void) {
			std::stringstream strm;
			strm << "[gameObject 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		// setNode isn't a member function, since it needs to be able to set
		// the shared pointer parent
		gameObject::ptr getNode(std::string name);
		void removeNode(std::string name);
		bool hasNode(std::string name);

		virtual glm::mat4 getTransform(void) {
			return glm::translate(position)
				* glm::mat4_cast(rotation)
				* glm::scale(scale);
		}

		// used for routing click events
		size_t id = allocateObjID();
		// TODO: bounding box/radius

		gameObject::ptr parent = nullptr;
		std::map<std::string, gameObject::ptr> nodes;
		uint64_t physics_id = 0; // for unlinking when the object is removed

		// relative to parent
		glm::vec3 position = glm::vec3(0, 0, 0);
		glm::quat rotation = glm::quat(1, 0, 0, 0);
		glm::vec3 scale    = glm::vec3(1, 1, 1);
		GLenum face_order  = GL_CCW;
};

static inline
void setNode(std::string name, gameObject::ptr obj, gameObject::ptr sub) {
	assert(obj != nullptr && sub != nullptr);

	obj->nodes[name] = sub;
	sub->parent = obj;
}

/*
// defined in gameModel.hpp
class gameMesh : public gameObject {
	public:
		typedef std::shared_ptr<gameModel> ptr;
		typedef std::weak_ptr<gameModel> weakptr;

		gameModel() : gameObject(objType::Model) {};
		std::string meshName = "unit_cube:default";
};

class gameModel : public gameObject {
	public:
		typedef std::shared_ptr<gameModel> ptr;
		typedef std::weak_ptr<gameModel> weakptr;

		gameModel() : gameObject(objType::Model) {};
		std::string modelName = "unit_cube";
		std::string sourceFile = "";
		Program::ptr shader = nullptr;
};
*/


class gameLight : public gameObject {
	public:
		typedef std::shared_ptr<gameLight> ptr;
		typedef std::weak_ptr<gameLight> weakptr;

		enum lightTypes {
			None,
			Point,
			Spot,
			Directional,
			Rectangular,  // TODO:
			Area,         // TODO:
			BoundedPoint, // TODO:
		} lightType;

		gameLight(enum lightTypes t)
			: gameObject(objType::Light), lightType(t) {};

		virtual std::string typeString(void) {
			std::stringstream strm;
			strm << "[Light (abstract) 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		virtual float extent(float threshold=0.03) = 0;
		// TODO: 'within(threshold, pos)' to test if a light affects a given point

		glm::vec4 diffuse = glm::vec4(1);
		float     intensity = 50.0;
		bool      casts_shadows = false;
		bool changed   = true;
		bool is_static = true;
		bool have_map  = false;
};

class gameLightPoint : public gameLight {
	public:
		typedef std::shared_ptr<gameLightPoint> ptr;
		typedef std::weak_ptr<gameLightPoint> weakptr;

		gameLightPoint() : gameLight(lightTypes::Point) {};

		virtual std::string typeString(void) {
			std::stringstream strm;
			strm << "[Point light 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		virtual float extent(float threshold=0.03);

		float radius = 1.0f;
		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap[6];
};

class gameLightSpot : public gameLight {
	public:
		typedef std::shared_ptr<gameLightSpot> ptr;
		typedef std::weak_ptr<gameLightSpot> weakptr;

		gameLightSpot() : gameLight(lightTypes::Spot) {};

		virtual std::string typeString(void) {
			std::stringstream strm;
			strm << "[Spot light 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		virtual float extent(float threshold=0.03);

		glm::vec3 direction;
		float radius;
		float angle;

		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap;
};

class gameLightDirectional : public gameLight {
	public:
		typedef std::shared_ptr<gameLightDirectional> ptr;
		typedef std::weak_ptr<gameLightDirectional> weakptr;

		gameLightDirectional() : gameLight(lightTypes::Directional) {};

		virtual std::string typeString(void) {
			std::stringstream strm;
			strm << "[Directional light 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		virtual float extent(float threshold=0.03);

		glm::vec3 direction;

		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap;
};

class gameReflectionProbe : public gameObject {
	public:
		typedef std::shared_ptr<gameReflectionProbe> ptr;
		typedef std::weak_ptr<gameReflectionProbe> weakptr;

		virtual std::string typeString(void) {
			std::stringstream strm;
			strm << "[Reflection probe 0x" << std::hex << this <<  "]";
			return strm.str();
		}

		gameReflectionProbe() : gameObject(objType::ReflectionProbe) {};
		quadtree::node_id faces[6];
		bool changed = true;
		bool is_static = true;
		bool have_map = false;
};

// namespace grendx
}
