#pragma once

#include <grend/gameMain.hpp>

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <utility>

using namespace grendx;

class levelController {
	gameMain *game = nullptr;

	public:
		typedef std::function<void(gameMain *game)> initFunc;
		typedef std::function<bool(gameMain *game)> objectiveFunc;
		typedef std::function<std::pair<bool, std::string>(gameMain *game)> loseFunc;
		typedef std::function<void(gameMain *game)> destructFunc;

		levelController() {};
		levelController(gameMain *_game) : game(_game) {};

		void addInit(initFunc func);
		void addDestructor(destructFunc func);
		void addLoseCondition(loseFunc func);
		void addObjective(std::string description, objectiveFunc func);

		void reset(void);
		bool won(void);                          // all objectives completed
		std::pair<bool, std::string> lost(void); // any lose condition is true

		std::vector<initFunc>     initializers;
		std::vector<destructFunc> destructors;
		std::vector<loseFunc>     loseConditions;

		std::map<std::string, objectiveFunc> objectives;
		std::map<std::string, bool>          objectivesCompleted;
};
