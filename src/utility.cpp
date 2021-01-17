#include <grend/utility.hpp>
#include <sstream>
#include <fstream>

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


// namespace grendx
}
