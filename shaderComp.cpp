#include <iostream>
#include <grend/utility.hpp>
#include <grend/shaderPreprocess.hpp>
#include <grend/glslObject.hpp>
#include <grend/logger.hpp>

using namespace grendx;

int main(int argc, char *argv[]) {
	std::string source    = load_file(argv[1]);
	//std::string processed = preprocessShader(source, options);
	//std::cout << processed << std::endl;

	try {
		auto t = glslObject(source);
		//auto t = parseGlsl(source);
		//dump_tokens_tree(t);

		/*
		if (!t.empty()) {
			LogInfo("Parsed shader successfully");
		} else {
			LogInfo("Couldn't parse shader.");
		}
		*/

	} catch (std::exception& e) {
		LogErrorFmt("Exception, couldn't parse shader: {}", e.what());
	}

	return 0;
}
