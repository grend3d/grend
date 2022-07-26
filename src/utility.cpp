#include <grend/utility.hpp>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <cxxabi.h>

namespace grendx {

std::vector<std::string> split_string(std::string s, char delim) {
	std::vector<std::string> ret;
	std::size_t pos = std::string::npos, last = 0;

	for (pos = s.find(delim); pos != std::string::npos; pos = s.find(delim, pos + 1)) {
		ret.push_back(s.substr(last, pos - last));
		last = pos + 1;
	}

	ret.push_back(s.substr(last));
	return ret;
}

std::string load_file(const std::string filename) {
	std::ifstream ifs(filename);
	std::string content((std::istreambuf_iterator<char>(ifs)),
	                    (std::istreambuf_iterator<char>()));

	return content;
}

// keep global map of type names over the whole lifetime of the engine,
// the number of names should be bounded by the number of classes in the program,
// so shouldn't be an issue...
static std::unordered_map<const char *, std::string> typemap;
static std::unordered_map<std::string, const char *> mangledmap;

const std::string& demangle(const char *type) {
	auto it = typemap.find(type); 
	if (it != typemap.end()) {
		return it->second;
	}

	// NOTE: targeting GCC and clang, no idea if this will work anywhere else
	int status;
	char *realname = abi::__cxa_demangle(type, 0, 0, &status);

	if (status == 0) {
		std::string ret = realname;
		typemap.insert({ type, ret });
		mangledmap.insert({ ret, type });
		free(realname);
		return typemap[type];
		//return ret;
	}

	static std::string invalid_name = "<invalid name>";
	return invalid_name;
}

const char *remangle(const std::string& demang) {
	auto it = mangledmap.find(demang);

	if (it != mangledmap.end()) {
		return it->second;
	}

	return NULL;
}

// namespace grendx
}
