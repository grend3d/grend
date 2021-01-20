#include <grend/shaderPreprocess.hpp>
#include <grend/glManager.hpp>
#include <grend/engine.hpp>
#include <grend/utility.hpp>
#include <sstream>
#include <exception>

using namespace grendx;

static std::string extractInclude(std::string pathspec) {
	// TODO: handle quoted paths
	size_t begin = pathspec.find('<');
	size_t end   = pathspec.find('>');

	if (begin == std::string::npos || end == std::string::npos || end < begin) {
		std::cerr << "Error: Invalid include specification! "
			<< pathspec << std::endl;
		return "/* Invalid include specification! */\n";
	}

	return pathspec.substr(begin + 1, end - begin - 1);
}

static std::string preprocess(std::string& sourcestr,
                              std::set<std::string>& included);

static std::string preprocessInclude(std::string& path,
                                     std::set<std::string>& included)
{

	if (!included.count(path)) {
		// TODO: need to be able to specify paths to search for shaders in
		std::string fullpath = GR_PREFIX + std::string("shaders/") + path;
		std::string source = load_file(fullpath);
		included.insert(path);

		std::string note = "// include from " + path + "\n";
		return note + preprocess(source, included);

	} else {
		return "// (already seen) include from " + path + "\n";
	}
}

static std::string preprocess(std::string& sourcestr,
                              std::set<std::string>& included)
{
	std::stringstream source(sourcestr);
	std::string processed;
	
	std::string line;
	while (std::getline(source, line)) {
		size_t inc = line.find("#include");
		size_t prag = line.find("#pragma");
		size_t directive = line.find("#");

		if (inc != std::string::npos) {
			auto path = extractInclude(line);
			processed += preprocessInclude(path, included);

		} else if (prag != std::string::npos) {
			// strip pragmas, just in case, they're leftovers from
			// the old preprocessor setup
			continue;

			/*
		} else if (directive != std::string::npos) {
			// remove leading spaces, if any
			processed += line.substr(directive) + "\n";
			*/

		} else {
			processed += line + "\n";
			continue;
		}
	}

	return processed;
}

std::string grendx::preprocessShader(std::string& source, shaderOptions& opts) {
	std::string version = std::string("#version ") + GLSL_STRING + "\n";

	std::string defines =
		"#define MAX_LIGHTS " + std::to_string(MAX_LIGHTS) + "\n" +
		"#define GLSL_VERSION " + std::to_string(GLSL_VERSION) + "\n" +
		"\n";

	std::set<std::string> includes;
	std::string processed = preprocess(source, includes);

	return version + defines + processed;
}
