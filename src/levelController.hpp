#pragma once

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <utility>

class levelController {
	public:
		typedef std::function<void()> initFunc;
		typedef std::function<bool()> objectiveFunc;
		typedef std::function<std::pair<bool, std::string>()> loseFunc;
		typedef std::function<void()> destructFunc;

		levelController() {};

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
