#pragma once

#include <grend/gameObject.hpp>
#include <grend/boundingBox.hpp>
#include <grend/ecs/ecs.hpp>
#include <vector>

using namespace grendx;
using namespace grendx::ecs;

class area : public component {
	public:
		area(entityManager *manager, entity *ent)
			: component(manager, ent)
		{
			manager->registerComponent(ent, "area", this);
		}

		virtual ~area();
		virtual bool inside(entityManager *manager, entity *ent, glm::vec3 pos) = 0;
};

class areaSphere : public area {
	float radius;

	public:
		areaSphere(entityManager *manager, entity *ent, float _radius)
			: area(manager, ent),
			  radius(_radius)
		{
			manager->registerComponent(ent, "areaSphere", this);
		}

		virtual ~areaSphere();
		virtual bool inside(entityManager *manager, entity *ent, glm::vec3 pos);
};

class areaAABB : public area {
	AABB box;

	public:
		areaAABB(entityManager *manager, entity *ent, AABB _box)
			: area(manager, ent),
			  box(_box)
		{
			manager->registerComponent(ent, "areaAABB", this);
		}

		virtual ~areaAABB();
		virtual bool inside(entityManager *manager, entity *ent, glm::vec3 pos);
};

class areaEvent : public component {
	public:
		areaEvent(entityManager *manager, entity *ent,
		          std::vector<std::string> _tags)
			: component(manager, ent),
			  tags(_tags)
		{
			manager->registerComponent(ent, "areaEvent", this);
		}

		virtual ~areaEvent();
		virtual void onEvent(entityManager *manager, entity *ent, entity *other) = 0;
		virtual void dispatch(entityManager *manager, entity *ent, entity *other) = 0;

		std::vector<std::string> tags;
		bool currentlyInside = false;
		bool lastInside = false;
};

class areaEnter : public areaEvent {
	public:
		areaEnter(entityManager *manager, entity *ent,
		          std::vector<std::string> _tags)
			: areaEvent(manager, ent, _tags)
		{
			manager->registerComponent(ent, "areaEnter", this);
		}

		virtual ~areaEnter();
		virtual void onEvent(entityManager *manager, entity *ent, entity *other) = 0;
		virtual void dispatch(entityManager *manager, entity *ent, entity *other);
};

class areaLeave : public areaEvent {
	public:
		areaLeave(entityManager *manager, entity *ent,
		          std::vector<std::string> _tags)
			: areaEvent(manager, ent, _tags)
		{
			manager->registerComponent(ent, "areaLeave", this);
		}

		virtual ~areaLeave();
		virtual void onEvent(entityManager *manager, entity *ent, entity *other) = 0;
		virtual void dispatch(entityManager *manager, entity *ent, entity *other);
};

class areaInside : public areaEvent {
	public:
		areaInside(entityManager *manager, entity *ent,
		           std::vector<std::string> _tags)
			: areaEvent(manager, ent, _tags)
		{
			manager->registerComponent(ent, "areaInside", this);
		}

		virtual ~areaInside();
		virtual void onEvent(entityManager *manager, entity *ent, entity *other) = 0;
		virtual void dispatch(entityManager *manager, entity *ent, entity *other);
};

class areaOutside : public areaEvent {
	public:
		areaOutside(entityManager *manager, entity *ent,
		           std::vector<std::string> _tags)
			: areaEvent(manager, ent, _tags)
		{
			manager->registerComponent(ent, "areaOutside", this);
		}

		virtual ~areaOutside();
		virtual void onEvent(entityManager *manager, entity *ent, entity *other) = 0;
		virtual void dispatch(entityManager *manager, entity *ent, entity *other);
};

class areaSystem : public entitySystem {
	public:
		typedef std::shared_ptr<areaSystem> ptr;
		typedef std::weak_ptr<areaSystem>   weakptr;

		virtual ~areaSystem();
		virtual void update(entityManager *manager, float delta);
};
