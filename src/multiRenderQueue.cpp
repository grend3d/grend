#include <grend/engine.hpp>
#include <grend/utility.hpp>
#include <grend/textureAtlas.hpp>
#include <math.h>

using namespace grendx;

void multiRenderQueue::add(const renderFlags& shader,
                           sceneNode::ptr obj,
                           uint32_t renderID,
                           const glm::mat4& trans,
                           bool inverted)
{
	size_t h = std::hash<renderFlags>{}(shader);

	if (shadermap.find(h) == shadermap.end()) {
		shadermap[h] = shader;
	}

	queues[h].add(obj, renderID, trans, inverted);
}

void grendx::cullQueue(multiRenderQueue& renque,
                       camera::ptr       cam,
                       unsigned          width,
                       unsigned          height,
                       float             lightext)
{
	for (auto& [id, que] : renque.queues) {
		cullQueue(que, cam, width, height, lightext);
	}
}

void grendx::sortQueue(multiRenderQueue& renque, camera::ptr cam) {
	for (auto& [id, que] : renque.queues) {
		sortQueue(que, cam);
	}
}

unsigned grendx::flush(multiRenderQueue&      que,
                       camera::ptr            cam,
                       renderFramebuffer::ptr fb,
                       renderContext          *rctx,
                       const renderOptions&   options)
{
	unsigned sum = 0;

	// TODO: flush in descending order depending on the number of
	//       objects using each shader
	for (auto& [id, flags] : que.shadermap) {
		sum += flush(que.queues[id], cam, fb, rctx, flags, options);
	}

	return sum;
}
