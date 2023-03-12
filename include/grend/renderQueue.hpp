#pragma once

#include <grend/renderFlags.hpp>
#include <grend/renderContext.hpp>
#include <grend/renderFramebuffer.hpp>

#include <grend/sceneNode.hpp>

namespace grendx {

void getNodeTransform(sceneNode::ptr obj,
                      glm::mat4 inTrans,
                      bool inInverted,
                      glm::mat4& outTrans,
                      bool& outInverted);

class renderQueue {
	public:
		renderQueue() {};

		renderQueue(renderQueue& other) {
			meshes        = other.meshes;
			skinnedMeshes = other.skinnedMeshes;
			lights        = other.lights;
			probes        = other.probes;
		}

		// TODO: version of this for particle queues
		template <typename T>
		struct queueEnt {
			glm::mat4 transform;
			glm::vec3 center;
			bool      inverted;
			T         data;
			uint32_t  renderID;
		};

		void add(sceneNode::ptr obj,
		         uint32_t renderID = 0,
		         glm::mat4 trans = glm::mat4(1),
		         bool inverted = false);

		void add(renderQueue& other);

		// returns 'true' if it's possible to recurse
		bool addNode(sceneNode::ptr obj,
		             uint32_t renderID = 0,
		             glm::mat4 trans = glm::mat4(1),
		             bool inverted = false);

		void addMesh(sceneNode::ptr obj,
		             uint32_t renderID = 0,
		             const glm::mat4& trans = glm::mat4(1),
		             bool inverted = false);

		void addSkinned(sceneNode::ptr obj,
		                sceneSkin::ptr skin,
		                uint32_t renderID = 0,
		                const glm::mat4& trans = glm::mat4(1),
		                bool inverted = false);
		void addInstanced(sceneNode::ptr obj,
		                  sceneParticles::ptr particles,
		                  uint32_t renderID = 0,
		                  const glm::mat4& outerTrans = glm::mat4(1),
		                  const glm::mat4& innerTrans = glm::mat4(1),
		                  bool inverted = false);
		void addBillboards(sceneNode::ptr obj,
		                   sceneBillboardParticles::ptr particles,
		                   uint32_t renderID = 0,
		                   const glm::mat4& trans = glm::mat4(1),
		                   bool inverted = false);

		void clear(void);

		using MeshQ  = std::vector<queueEnt<sceneMesh::ptr>>;
		using LightQ = std::vector<queueEnt<sceneLight::ptr>>;
		using RefQ   = std::vector<queueEnt<sceneReflectionProbe::ptr>>;
		using RadQ   = std::vector<queueEnt<sceneIrradianceProbe::ptr>>;
		using SkinQ  = std::map<sceneSkin::ptr, MeshQ>;

		// mat4 is calculated transform for the position of the node in the tree
		// bool is inverted flag
		MeshQ  meshes;
		MeshQ  meshesBlend;   // blend opacity
		MeshQ  meshesMasked;  // mask/clip opacity

		SkinQ  skinnedMeshes;
		LightQ lights;
		RefQ   probes;
		RadQ   irradProbes;

		// TODO: hmm, having types that line wrap might be a code smell...
		std::vector<std::tuple<glm::mat4, glm::mat4, bool,
		                       sceneParticles::ptr,
		                       sceneMesh::ptr>> instancedMeshes;
		std::vector<std::tuple<glm::mat4, bool, sceneBillboardParticles::ptr,
		                       sceneMesh::ptr>> billboardMeshes;

		sceneReflectionProbe::ptr nearest_reflection_probe(glm::vec3 pos);
		sceneIrradianceProbe::ptr nearest_irradiance_probe(glm::vec3 pos);
};

struct multiRenderQueue {
	public:
		std::unordered_map<std::size_t, renderQueue> queues;
		std::unordered_map<std::size_t, renderFlags> shadermap;

		// TODO: probably rename renderFlags to renderShader
		void add(const renderFlags& shader,
		         sceneNode::ptr obj,
		         uint32_t renderID = 0,
		         const glm::mat4& trans = glm::mat4(1),
		         bool inverted = false);

		void add(const renderFlags& shader, renderQueue& que);

		// TODO:
		//void add(multiRenderQueue& other);

		void clear(void);
};

void updateLights(renderContext *rctx, renderQueue& lights);
void updateReflections(renderContext *rctx, renderQueue& refs);
void updateReflectionProbe(renderContext *rctx, renderQueue& que, camera::ptr cam);
void sortQueue(renderQueue& queue, camera::ptr cam);
void cullQueue(renderQueue& queue, camera::ptr cam, unsigned width, unsigned height, float lightext);
void sortQueue(multiRenderQueue& queue, camera::ptr cam);
void cullQueue(multiRenderQueue& queue, camera::ptr cam, unsigned width, unsigned height, float lightext);
void batchQueue(renderQueue& queue);

void shaderSync(Program::ptr program, renderContext *rctx, renderQueue& que);

// TODO: rename flushQueue
// TODO: renderFlags -> renderShader, renderOptions -> renderFlags
unsigned flush(renderQueue& que,
               camera::ptr cam,
               renderFramebuffer::ptr fb,
               renderContext *rctx,
               const renderFlags& flags,
               const renderOptions& options);

unsigned flush(multiRenderQueue& que,
               camera::ptr cam,
               renderFramebuffer::ptr fb,
               renderContext *rctx,
               const renderOptions& options);

// TODO: this for multiRenderQueue
unsigned flush(renderQueue& que,
               camera::ptr cam,
               unsigned width,
               unsigned height,
               renderContext *rctx,
               const renderFlags& flags,
               const renderOptions& options);

// namespace grendx
}
