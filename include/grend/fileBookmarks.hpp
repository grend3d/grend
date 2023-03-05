#include <grend/IoC.hpp>

#include <iostream>
#include <fstream>
#include <list>
#include <string>
#include <string_view>
#include <optional>

namespace grendx {

struct bookmark {
	std::string name;
	std::string path;
};

// Wrapping state up as a service so that multiple things can use the same
// bookmark component while keeping state in sync
class fileBookmarksState : public IoC::Service {
	public:
		fileBookmarksState();

		const std::list<std::string>& getBookmarks();
		void removeBookmark(size_t index);
		void addBookmark(std::string_view path);
		void save(void);
		void load(void);

	private:
		std::list<std::string> bookmarks;
};

std::optional<std::string> fileBookmarks();

// namespace grendx
}
