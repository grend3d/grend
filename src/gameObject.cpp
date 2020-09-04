#include <grend/gameObject.hpp>
#include <math.h>

using namespace grendx;

size_t grendx::allocateObjID(void) {
	static size_t counter = 0;
	return ++counter;
}

gameObject::ptr gameObject::getNode(std::string name) {
	return hasNode(name)? nodes[name] : nullptr;
}

gameObject::ptr grendx::unlink(gameObject::ptr node) {
	if (node != nullptr && node->parent) {
		for (auto& [key, ptr] : node->parent->nodes) {
			if (node == ptr) {
				gameObject::ptr ret = node->parent;
				node->parent->nodes.erase(key);
				node->parent = nullptr;
				return ret;
			}
		}
	}

	return node;
}

gameObject::ptr grendx::clone(gameObject::ptr node) {
	gameObject::ptr ret = std::make_shared<gameObject>();

	ret->position = node->position;
	ret->scale    = node->scale;
	ret->rotation = node->rotation;

	for (auto& [name, ptr] : node->nodes) {
		setNode(name, ret, ptr);
	}

	return ret;
}

gameObject::ptr grendx::duplicate(gameObject::ptr node) {
	// TODO: need to copy all attributes
	/*
	gameObject::ptr ret = std::make_shared<gameObject>();

	for (auto& [name, ptr] : node->nodes) {
		setNode(name, ret, duplicate(ptr));
	}
	*/

	return clone(node);
}

void gameObject::removeNode(std::string name) {
	auto it = nodes.find(name);

	if (it != nodes.end()) {
		nodes.erase(it);
	}
}

bool gameObject::hasNode(std::string name) {
	return nodes.find(name) != nodes.end();
}

float gameLightPoint::extent(float threshold) {
	return radius * (sqrt((intensity * diffuse.w)/threshold) - 1);
}

float gameLightSpot::extent(float threshold) {
	return radius * (sqrt((intensity * diffuse.w)/threshold) - 1);
}

float gameLightDirectional::extent(float threshold) {
	// infinite extent
	return HUGE_VALF;
}
