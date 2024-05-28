#pragma once

#include <vector>
#include <string>
#include <stdint.h>
#include <string.h>

#include <filesystem>
namespace fs = std::filesystem;

namespace grendx {

enum paneTypes {
	Other,
	Directory,
	File,
};

struct paneNode {
	paneNode(fs::path path, enum paneTypes t)
		: fullpath(path),
	      type(t)
	{
		name = fullpath.filename();
	};

	void expand(void);

	fs::path fullpath;
	std::string name;
	std::vector<paneNode> subnodes;
	paneTypes type;

	bool expanded = false;
	bool selected = false;
	//uint64_t modified = 0;
	std::filesystem::file_time_type last_modified;
};

struct filePane {
	paneNode root;
	void *selected;

	bool editPath = false;
	char pathBuf[1024];

	bool rootUpdated = false;
	std::string newPathBuf;

	filePane(fs::path rootpath = fs::current_path())
		: root(rootpath, paneTypes::Directory)
	{
		updatePathBuf();
	}

	void render();
	void renderNodesRec(paneNode& node);
	void renderNodes(void);
	void reset(void);
	void chdir(std::string_view newroot);
	void setSelected(std::string_view path);

	void updatePathBuf(void);
};

// namespace grendx
}
