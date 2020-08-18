#pragma once
#include <grend/glm-includes.hpp>
#include <grend/opengl-includes.hpp>
#include <grend/sdl-context.hpp>
// TODO: move, for lights
#include <grend/quadtree.hpp>

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <utility>

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
		virtual void onLeftClick() {};
		virtual void onMiddleClick() {};
		virtual void onRightClick() {};
		virtual void onHover() {};

		void setNode(std::string name, gameObject::ptr obj);
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

		// relative to parent
		glm::vec3 position = glm::vec3(0, 0, 0);
		glm::quat rotation = glm::quat(1, 0, 0, 0);
		glm::vec3 scale    = glm::vec3(1, 1, 1);
};

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

		enum lightType {
			None,
			Point,
			Spot,
			Directional,
			Rectangular,  // TODO:
			Area,         // TODO:
			BoundedPoint, // TODO:
		} light_type;

		gameLight(enum lightType t)
			: gameObject(objType::Light), light_type(t) {};

		glm::vec4 diffuse = glm::vec4(1);
		float     intensity = 50.0;
		bool      casts_shadows = false;
};

class gameLightPoint : public gameLight {
	public:
		typedef std::shared_ptr<gameLightPoint> ptr;
		typedef std::weak_ptr<gameLightPoint> weakptr;

		gameLightPoint() : gameLight(lightType::Point) {};
		float radius;
		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap[6];
		bool changed = true;
		bool static_shadows = false;
		bool shadows_rendered = false;
};

class gameLightSpot : public gameLight {
	public:
		typedef std::shared_ptr<gameLightSpot> ptr;
		typedef std::weak_ptr<gameLightSpot> weakptr;

		gameLightSpot() : gameLight(lightType::Spot) {};
		glm::vec3 direction;
		float radius;
		float angle;

		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap;
		bool changed = true;
		bool static_shadows = false;
		bool shadows_rendered = true;
};

class gameLightDirectional : public gameLight {
	public:
		typedef std::shared_ptr<gameLightDirectional> ptr;
		typedef std::weak_ptr<gameLightDirectional> weakptr;

		gameLightDirectional() : gameLight(lightType::Directional) {};
		glm::vec3 direction;

		// TODO: maybe abstract atlas textures more
		quadtree::node_id shadowmap;
		bool changed = true;
		bool static_shadows = false;
		bool shadows_rendered = true;
};

class gameReflectionProbe : public gameObject {
	public:
		typedef std::shared_ptr<gameReflectionProbe> ptr;
		typedef std::weak_ptr<gameReflectionProbe> weakptr;

		gameReflectionProbe() : gameObject(objType::ReflectionProbe) {};
		quadtree::node_id faces[6];
		bool changed = true;
		bool is_static = true;
		bool have_map = false;
};

// namespace grendx
}
