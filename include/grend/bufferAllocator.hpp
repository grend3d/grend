#pragma once
#include <set>
#include <stdlib.h>
#include <stdint.h>

namespace grendx {

struct bufferNode {
	struct bufferNode *prev = nullptr;
	struct bufferNode *next = nullptr;

	enum states {
		// unavailable state used in sentinel node 
		Unavailable,
		Free,
		Used,
	} state = states::Unavailable;

	uintptr_t offset = 0;
	size_t size = 0;
};

class bufferAllocator {
	public:
		bufferAllocator();
		bufferNode *allocate(size_t amount);
		void free(bufferNode *ptr);
		unsigned alignment = 4;

	private:
		bufferNode *coalesce(bufferNode *ptr);

		bufferNode *start;
		bufferNode *end;

		std::set<bufferNode*> freeNodes;
		uintptr_t wilderness = 0;
};

// namespace grendx
}
