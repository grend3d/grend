#include "team.hpp"

team::~team() {};
bool team::operator==(const team& other) const {
	return name == other.name;
}
