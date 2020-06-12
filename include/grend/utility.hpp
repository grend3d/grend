#pragma once
#include <vector>
#include <string>

namespace grendx {

template<typename A, typename B>
static inline A min(A a, B b) {
	return (a < b)? a : b;
}

template<typename A, typename B>
static inline A max(A a, B b) {
	return (a > b)? a : b;
}

std::vector<std::string> split_string(std::string s, char delim=' ');

static inline std::string filename_extension(std::string fname) {
	return fname.substr(fname.rfind("."));
}

// namespace grendx
}
