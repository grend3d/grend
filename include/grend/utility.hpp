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
std::string load_file(const std::string filename);

static inline std::string filename_extension(std::string fname) {
	auto pos = fname.rfind(".");
	return (pos == std::string::npos)? "" : fname.substr(pos);
}

static inline std::string basenameStr(std::string filename) {
	std::size_t found = filename.rfind("/");

	if (found != std::string::npos) {
		return filename.substr(found+1, std::string::npos);
	}

	return filename;
}

static inline std::string dirnameStr(std::string filename) {
	std::size_t found = filename.rfind("/");

	if (found != std::string::npos) {
		return filename.substr(0, found);
	}

	return "./";
}

// namespace grendx
}
