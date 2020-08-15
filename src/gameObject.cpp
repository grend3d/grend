#include <grend/gameObject.hpp>

using namespace grendx;

size_t allocateObjID(void) {
	static size_t counter = 0;
	return ++counter;
}
