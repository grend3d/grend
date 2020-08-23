#include <grend/quadtree.hpp>
#include <grend/utility.hpp>
#include <math.h>
#include <iostream>
#include <stdio.h>

using namespace grendx;

quadtree::quadtree(size_t dim, unsigned block_sz) {
	dimension = dim;
	block_size = block_sz;

	root = new quadtree::node(0, 0, dim);
}

quadtree::node_id quadtree::alloc(unsigned size) {
	unsigned rounded_size = pow(2, ceil(log2(size)));

	// can't allocate anything larger than half the root dimension
	if (rounded_size > dimension/2 || size == 0) {
		return 0;
	}

	node_id ret = 0;
	node *retnode = nullptr;

	while (retnode == nullptr) {
		retnode = alloc_node(rounded_size);

		if (!retnode) {
			std::cerr << "XXX: freeing old nodes..." << std::endl;
			//assert(false);
			free_oldest();
		}
	}

	if (retnode) {
		ret = alloc_id();
		retnode->id = ret;
		active_nodes[ret] = retnode;
	}

	return ret;
}

quadtree::node *quadtree::alloc_node(unsigned size) {
	node *ret = nullptr;
	node *temp = root;
	node *iter = nullptr;

	unsigned cur_x = 0;
	unsigned cur_y = 0;

	while (temp && !ret) {
		iter = nullptr;

		if (size > temp->max_free) {
			break;
		}

		for (unsigned i = 0; i < 4; i++) {
			bool x = i&1;
			bool y = i&2;
			unsigned off_x = cur_x + x*temp->size/2;
			unsigned off_y = cur_y + y*temp->size/2;

			fprintf(stderr, "i: %u, size: %u, max_free: %u, x: %u, y: %u, off_x: %u, off_y: %u\n",
				i, temp->size, temp->max_free, x, y, off_x, off_y);

			if (temp->subnodes[x][y] && size <= temp->subnodes[x][y]->max_free) {
				iter = temp->subnodes[x][y];
				cur_x = off_x;
				cur_y = off_y;
				break;

			} else if (!temp->subnodes[x][y] && size < temp->max_free) {
				iter = temp->subnodes[x][y] =
					new node(off_x, off_y, temp->size/2, temp);
				cur_x = off_x;
				cur_y = off_y;
				break;

			} else if (!temp->subnodes[x][y] && size == temp->max_free) {
				ret = temp->subnodes[x][y] =
					new node(off_x, off_y, temp->size/2, temp);
				ret->stamp = alloc_stamp();
				ret->update_tree();
				break;
			}
		}

		temp = iter;
	}

	fprintf(stderr, "allocated node %p\n", ret);

	return ret;
}

// conditions to ensure when freeing:
// - node is valid before freeing, invalidated after
//   (remove from active_nodes)
// - recalculate parent's max_free + stamp, propagate upwards
// - if parent node group is empty, set parent stamp to -1, max_free to level
//   size, repeat for grandparent etc until not empty or the top is reached
void quadtree::free(node_id id) {
	if (!valid(id)) {
		return;
	}

	node *ptr = active_nodes[id];
	assert(ptr->parent != nullptr);

	while (ptr->is_leaf() && ptr->parent) {
		node *parent = ptr->parent;
		parent->unlink_subnode(ptr);
		delete ptr;

		ptr = ptr->parent;
	}

	assert(ptr != nullptr);
	ptr->update_tree();
	active_nodes.erase(id);
}

void quadtree::free_oldest(void) {
	if (!root) return;
	node *temp = root;

	while (!temp->is_leaf()) {
		temp = temp->min_stamp_node();
		assert(temp != nullptr);
	}

	if (temp->id == 0 || !valid(temp->id)) {
		// TODO: might want to do something here
		assert(temp != 0);
		assert(valid(temp->id));
		return;
	}

	fprintf(stderr, "freeing ID %u\n", temp->id);
	free(temp->id);
}

quadinfo quadtree::info(node_id id) {
	if (!valid(id)) {
		return (quadinfo) { .valid = false };
	}

	node *ptr = active_nodes[id];

	return (quadinfo) {
		.x = ptr->x,
		.y = ptr->y,
		.size = ptr->size,
		.dimension = dimension,
		.valid = true,
	};
}

quadtree::node_id quadtree::refresh(node_id id) {
	if (!valid(id)) {
		return 0;
	}

	node *ptr = active_nodes[id];
	ptr->stamp = alloc_stamp();
	ptr->update_tree();

	assert(ptr->id == id);
	return id;
}

bool quadtree::valid(node_id id) {
	return active_nodes.find(id) != active_nodes.end();
}

quadtree::node::node(unsigned nx, unsigned ny, unsigned sz, node *p) {
	x = nx;
	y = ny;
	size = sz;
	parent = p;
	
	max_free = sz/2;

	for (unsigned i = 0; i < 4; i++) {
		subnodes[!!(i&1)][!!(i&2)] = nullptr;
	}
}

bool quadtree::node::is_leaf(void) {
	for (unsigned i = 0; i < 4; i++) {
		bool x = i&1;
		bool y = i&2;

		if (subnodes[x][y]) {
			return false;
		}
	}

	return true;
}

quadtree::cache_stamp quadtree::node::min_stamp(void) {
	quadtree::cache_stamp minstamp = (quadtree::cache_stamp)-1;

	if (is_leaf()) {
		return stamp;
	}

	for (unsigned i = 0; i < 4; i++) {
		bool x = i&1;
		bool y = i&2;

		if (subnodes[x][y]) {
			minstamp = min(minstamp, subnodes[x][y]->stamp);
		}
	}

	return minstamp;
};

quadtree::node *quadtree::node::min_stamp_node(void) {
	quadtree::cache_stamp minstamp = (quadtree::cache_stamp)-1;
	node *ret = nullptr;

	for (unsigned i = 0; i < 4; i++) {
		bool x = i&1;
		bool y = i&2;

		if (subnodes[x][y] && subnodes[x][y]->stamp < minstamp) {
			minstamp = subnodes[x][y]->stamp;
			ret = subnodes[x][y];
		}
	}

	return ret;
};

void quadtree::node::unlink_subnode(node *ptr) {
	for (unsigned i = 0; i < 4; i++) {
		bool x = i&1;
		bool y = i&2;

		if (subnodes[x][y] == ptr) {
			subnodes[x][y] = nullptr;
		}
	}
}

unsigned quadtree::node::get_max_free(void) {
	unsigned ret = 0;

	if (is_leaf()) {
		return 0;
	}

	for (unsigned i = 0; i < 4; i++) {
		bool x = i&1;
		bool y = i&2;

		ret = max(ret, subnodes[x][y]? subnodes[x][y]->max_free : (size / 2));
	}

	return ret;
}

void quadtree::node::update_tree(void) {
	for (node *p = this; p; p = p->parent) {
		p->max_free = p->get_max_free();
		p->stamp = p->min_stamp();

		/*
		// leaving this here in case strange bugs pop up in the quadtree...
		fprintf(stderr, "update at %p: max free: %u, stamp: %lu\n",
				p, p->max_free, p->stamp);
				*/
	}
}
