#pragma once

#include <grend/sceneNode.hpp>
#include <grend/animation.hpp>
#include <grend/loadScene.hpp>

#include <grend/ecs/ecs.hpp>

using namespace grendx::ecs;

namespace grendx {

class animationController
	: public component,
	  public updatable
{
	public:
		typedef std::shared_ptr<animationController> ptr;
		typedef std::weak_ptr<animationController>   weakptr;

		animationController(regArgs t);
		animationController(regArgs t, animationCollection::ptr anims);

		void setAnimation(std::string animation, float weight = 1.0);
		void update(ecs::entityManager *manager, float delta);

		// bind an animation to a name
		// the animation can be from another animation collection
		void bind(std::string name, animationMap::ptr anim);
		void bind(std::string name, std::string path);
		void setSpeed(float speed);

		static nlohmann::json serializer(component *comp);
		static void deserializer(component *comp, nlohmann::json j);

		static void drawEditor(component *ent);

	private:
		float animTime = 0;
		float animSpeed = 1.0;

		// TODO: std filesystem
		std::vector<std::pair<std::string, std::string>> paths;

		animationCollection::ptr animations;
		animationMap::ptr currentAnimation;
};

void applyAnimation(sceneNode::ptr node, animationMap::ptr anim, float time);

// namespace grendx
}
