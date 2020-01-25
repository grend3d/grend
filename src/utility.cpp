#include <grend/utility.hpp>

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

// namespace grendx
}
