#include <grend/octree.hpp>
#include <math.h>

using namespace grendx;

void octree::grow(double size) {
	if (size < leaf_size * (1 << levels)) {
		// tree is already big enough, nothing to do
		return;
	}

	if (root == nullptr) {
		// special case, if there currently aren't any nodes,
		// the level can be bumped up enough to fit the requested size
		puts("asdf");
		levels = log2(size/leaf_size);
		return;
	}

	while (size >= leaf_size * (1 << levels)) {
		for (unsigned i = 0; i < 8; i++) {
			bool x = i&1;
			bool y = i&2;
			bool z = i&4;

			if (root->subnodes[x][y][z] == nullptr) {
				continue;
			}

			node *subtemp = new node;
			subtemp->subnodes[!x][!y][!z] = root->subnodes[x][y][z];
			subtemp->level = levels;
			root->subnodes[x][y][z] = subtemp;
		}

		root->level = ++levels;
	}
}

/*
// old (stub) function, left here for debugging and comparison
void octree::add_tri(const glm::vec3 tri[3]) {
	const glm::vec3& a = tri[0], b = tri[1], c = tri[2];

	glm::vec3 foo  = (a + b) / 2.f;
	glm::vec3 bar  = (a + c) / 2.f;
	glm::vec3 baz  = (b + c) / 2.f;

	// XXX: for now just set leaves at the middle of each face
	set_leaf(foo);
	set_leaf(bar);
	set_leaf(baz);
}
*/

void octree::add_tri(const glm::vec3 tri[3]) {
	const glm::vec3& a = tri[0], b = tri[1], c = tri[2];

	glm::vec3 plane_normal =
		glm::normalize(glm::cross(
			b - a,
			c - a));

	// direction perpendicular to the base in the triangle's plane
	glm::vec3 perp = glm::cross(plane_normal, glm::normalize(c - b));

	float la = glm::length((b - a));
	float lb = glm::length((c - a));
	float lc = glm::length((c - b));

	// semi-perimeter
	float s = (la + lb + lc) / 2;
	// altitude from the BC ("c") side
	float altitude = 2 * sqrt(s * (s-la) * (s-lb) * (s-lc)) / lc;

	float B = glm::dot(b - a, b - c);
	float C = glm::dot(c - a, c - b);

	float r1 = glm::sec(B);
	float r2 = glm::sec(C);

	glm::vec3 basedir = glm::normalize(c - b);
	float k = lc / altitude;

	for (float i = 0; i < altitude; i += leaf_size) {
		glm::vec3 center = a - perp*i;
		set_leaf(center);

		for (float m = 0; m < i*k*r1; m += leaf_size) {
			set_leaf(center - basedir*m);
		}

		for (float m = 0; m < i*k*r2; m += leaf_size) {
			set_leaf(center + basedir*m);
		}
	}
}

// TODO: move to utilities header
#define max(A, B) (((A) > (B))? (A) : (B))

void octree::set_leaf(glm::vec3 location) {
	// TODO: this probably gets pretty slow, might as well calculate bounding
	//       boxes for models so the size only needs to be calculated once
	double maxsize = max(max(fabs(location.x), fabs(location.y)), fabs(location.z));
	grow(maxsize);

	if (!root) {
		root = new node;
	}

	node *move = root;
	for (unsigned level = levels; level--;) {
		//printf("%u: %f, %f, %f\n\n", level, location.x, location.y, location.z);
		bool x = location.x < 0;
		bool y = location.y < 0;
		bool z = location.z < 0;

		if (move->subnodes[x][y][z] == nullptr) {
			move->subnodes[x][y][z] = new node;
			move->subnodes[x][y][z]->level = level;
		}

		location.x -= (x?-1:1) * leaf_size * (1 << level);
		location.y -= (y?-1:1) * leaf_size * (1 << level);
		location.z -= (z?-1:1) * leaf_size * (1 << level);

		move = move->subnodes[x][y][z];
	}
	//puts("set leaf");
}

uint32_t octree::count_nodes(void) {
	if (!root) {
		return 0;
	}

	return 1 + root->count_nodes();
}

uint32_t octree::node::count_nodes(void) {
	uint32_t sum = 0;

	for (unsigned i = 0; i < 8; i++) {
		node *ptr = subnodes[!!(i&1)][!!(i&2)][!!(i&4)];

		if (ptr == nullptr) {
			continue;
		}

		sum += 1 + ptr->count_nodes();
	}

	return sum;
}
