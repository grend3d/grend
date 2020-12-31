#pragma once


#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>

using namespace grendx;
using namespace grendx::ecs;

struct generatorEvent {
	// event interface for ECS stuff, this way entities can be
	// spawned/despawned/deactivated/etc as the player moves around
	enum types {
		generatorStarted,
		generated,
		deleted,
	} type;

	glm::vec3 position;
	glm::vec3 extent;
};

// TODO: maybe mutex, although events probably won't be pushed from workers here
typedef std::shared_ptr<std::vector<generatorEvent>> generatorEventQueue;

class worldGenerator {
	public:
		virtual gameObject::ptr getNode(void) { return root; };
		virtual void setPosition(gameMain *game, glm::vec3 position) = 0;
		virtual void setEventQueue(generatorEventQueue q);

	protected:
		// TODO: generic event class
		virtual void emit(generatorEvent ev);

		gameObject::ptr root = std::make_shared<gameObject>();
		glm::vec3 lastPosition = glm::vec3(INT_MAX);
		generatorEventQueue eventQueue = nullptr;
};

class landscapeGenerator : public worldGenerator {
	public:
		landscapeGenerator(unsigned seed = 0xcafebabe);
		virtual void setPosition(gameMain *game, glm::vec3 position);

	private:
		void generateLandscape(gameMain *game, glm::vec3 curpos, glm::vec3 lastpos);
		std::future<bool> genjob;
		gameObject::ptr returnValue;
};

// XXX: global variable, TODO: something else
//      (could just put it in the landscapeGenerator contructor)
inline material::ptr landscapeMaterial;
inline gameObject::ptr landscapeNodes = std::make_shared<gameObject>();
inline gameModel::ptr treeNode;

