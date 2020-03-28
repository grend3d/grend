#pragma once
#include <vector>
#include <string>

namespace grendx {

static inline double min(double a, double b) {
	return (a < b)? a : b;
}

static inline double max(double a, double b) {
	return (a > b)? a : b;
}

std::vector<std::string> split_string(std::string s, char delim=' ');

// namespace grendx
}
