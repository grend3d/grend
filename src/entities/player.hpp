#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

#include <memory>
#include <vector>

#include <components/boxSpawner.hpp>
#include <components/inputHandler.hpp>

using namespace grendx;
using namespace grendx::ecs;

animationCollection::ptr findAnimations(gameObject::ptr obj);

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

class player : public entity {
	public:
		player(entityManager *manager, gameMain *game, glm::vec3 position);
		player(entityManager *manager, entity *ent, nlohmann::json properties);
		virtual ~player();

		virtual void update(entityManager *manager, float delta);
		virtual gameObject::ptr getNode(void) { return node; };

		animatedCharacter::ptr character;

		// serialization stuff
		constexpr static const char *serializedType = "player";
		static const nlohmann::json defaultProperties(void) {
			return entity::defaultProperties();
		}

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager); 
};
