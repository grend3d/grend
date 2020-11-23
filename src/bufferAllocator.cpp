#include <grend/bufferAllocator.hpp>

using namespace grendx;

static bufferNode sentinel = {
	.prev = &sentinel,
	.next = &sentinel,
};

bufferAllocator::bufferAllocator() {
	start = end = &sentinel;
}

bufferNode *bufferAllocator::allocate(size_t amount) {
	for (auto it = freeNodes.begin(); it != freeNodes.end(); it++) {
		// node is larger than needed, split into two chunks
		// TODO: split() function
		if ((*it)->size >= amount) {
			bufferNode *ret = (*it);

			if (ret->size > amount) {
				// TODO: align amount
				bufferNode *temp = new bufferNode;
				temp->size = ret->size - amount;
				temp->offset = ret->offset + amount;
				temp->prev = ret;
				temp->next = ret->next;

				if (temp->next != &sentinel) {
					temp->next->prev = temp;
				}

				temp->state = bufferNode::states::Free;
				freeNodes.insert(temp);

				ret->next = temp;
				ret->size = amount;
			}

			ret->state = bufferNode::states::Used;
			freeNodes.erase(ret);
			return ret;
		}
	}

	// TODO: align amount
	bufferNode *ret = new bufferNode;
	ret->size = amount;
	ret->offset = wilderness;
	ret->state = bufferNode::states::Used;

	ret->prev = end;
	ret->next = &sentinel;

	if (end != &sentinel) {
		end->next = ret;
	}

	end = ret;
	wilderness += amount;

	return ret;
}

void bufferAllocator::free(bufferNode *ptr) {
	if (!ptr || ptr == &sentinel) {
		return;
	}

	ptr->state = bufferNode::states::Free;
	freeNodes.insert(coalesce(ptr));
}

bufferNode *bufferAllocator::coalesce(bufferNode *ptr) {
	while (ptr->prev->state == bufferNode::states::Free) {
		bufferNode *prev = ptr->prev;

		ptr->size   += prev->size;
		ptr->offset  = prev->offset;
		ptr->prev    = prev->prev;

		if (ptr->prev != &sentinel) {
			ptr->prev->next = ptr;
		}

		prev->state = bufferNode::states::Unavailable;
		freeNodes.erase(prev);
		delete prev;

		if (prev == end) {
			end = ptr;
		}
	}

	while (ptr->next->state == bufferNode::states::Free) {
		bufferNode *next = ptr->next;

		ptr->size += next->size;
		ptr->next  = next->next;

		if (ptr->next != &sentinel) {
			ptr->next->prev = ptr;
		}

		next->state = bufferNode::states::Unavailable;
		freeNodes.erase(next);
		delete next;

		if (next == end) {
			end = ptr;
		}
	}

	return ptr;
}
