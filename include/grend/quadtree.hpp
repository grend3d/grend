#pragma once

#include <grend/glm-includes.hpp>
#include <grend/model.hpp>

#include <stdint.h>

namespace grendx {

struct quadinfo {
	unsigned x;
	unsigned y;
	unsigned size;
	unsigned dimension;
	bool valid;
};

class quadtree {
	public:
		typedef uint32_t node_id;
		typedef uint64_t cache_stamp;
		class node;

		// underlying texture must be a square with power of two sides
		quadtree(size_t dimension, unsigned block_sz=16);
		~quadtree() { };

		node_id alloc(unsigned block);
		node_id alloc_pinned(unsigned block);
		node_id alloc_no_free(unsigned block);
		node*   alloc_node(unsigned size);
		void free(node_id id);
		void free_oldest(void);

		quadinfo info(node_id id);
		node_id refresh(node_id id);
		bool valid(node_id id);
		node_id alloc_id(void) { return alloced_ids++; };
		cache_stamp alloc_stamp(void) { return cache_counter++; };

	private:
		node *root = nullptr;
		std::map<node_id, node*> active_nodes;

		unsigned dimension;
		unsigned block_size;
		node_id alloced_ids = 1;
		cache_stamp cache_counter = 1;
};

class quadtree::node {
	public:
		node(unsigned x, unsigned y, unsigned size, node *p = nullptr);

		bool is_leaf(void);
		quadtree::cache_stamp min_stamp(void);
		quadtree::node *min_stamp_node(void);
		void unlink_subnode(node *ptr);
		unsigned get_max_free(void);
		void update_tree(void);

		unsigned x, y, size;
		unsigned max_free;

		node *parent = nullptr;
		node *subnodes[2][2] = {0};
		bool pinned = false;;

		quadtree::node_id id;
		quadtree::cache_stamp stamp;
};

// namespace grendx
}
