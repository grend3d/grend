#pragma once

#include <grend/renderFlags.hpp>
#include <grend/renderContext.hpp>
#include <grend/renderFramebuffer.hpp>

#include <grend/gameObject.hpp>

namespace grendx {

class renderQueue {
	public:
		renderQueue() {};

		renderQueue(renderQueue& other) {
			meshes        = other.meshes;
			skinnedMeshes = other.skinnedMeshes;
			lights        = other.lights;
			probes        = other.probes;
		}

		template <typename T>
		struct queueEnt {
			glm::mat4 transform;
			glm::vec3 center;
			bool      inverted;
			T         data;
		};

		void add(gameObject::ptr obj,
		         glm::mat4 trans = glm::mat4(1),
		         bool inverted = false);

		void add(renderQueue& other);

		void addSkinned(gameObject::ptr obj,
		                gameSkin::ptr skin,
		                glm::mat4 trans = glm::mat4(1),
		                bool inverted = false);
		void addInstanced(gameObject::ptr obj,
		                  gameParticles::ptr particles,
		                  glm::mat4 outerTrans = glm::mat4(1),
		                  glm::mat4 innerTrans = glm::mat4(1),
		                  bool inverted = false);
		void addBillboards(gameObject::ptr obj,
		                   gameBillboardParticles::ptr particles,
		                   glm::mat4 trans = glm::mat4(1),
		                   bool inverted = false);

		void clear(void);

		using MeshQ  = std::vector<queueEnt<gameMesh::ptr>>;
		using LightQ = std::vector<queueEnt<gameLight::ptr>>;
		using RefQ   = std::vector<queueEnt<gameReflectionProbe::ptr>>;
		using RadQ   = std::vector<queueEnt<gameIrradianceProbe::ptr>>;
		using SkinQ  = std::map<gameSkin::ptr, MeshQ>;

		// mat4 is calculated transform for the position of the node in the tree
		// bool is inverted flag
		MeshQ  meshes;
		SkinQ  skinnedMeshes;
		LightQ lights;
		RefQ   probes;
		RadQ   irradProbes;

		// TODO: hmm, having types that line wrap might be a code smell...
		std::vector<std::tuple<glm::mat4, glm::mat4, bool,
		                       gameParticles::ptr,
		                       gameMesh::ptr>> instancedMeshes;
		std::vector<std::tuple<glm::mat4, bool, gameBillboardParticles::ptr,
		                       gameMesh::ptr>> billboardMeshes;

		gameReflectionProbe::ptr nearest_reflection_probe(glm::vec3 pos);
		gameIrradianceProbe::ptr nearest_irradiance_probe(glm::vec3 pos);
};

struct multiRenderQueue {
	public:
		std::unordered_map<std::size_t, renderQueue> queues;
		std::unordered_map<std::size_t, renderFlags> shadermap;

		// TODO: probably rename renderFlags to renderShader
		void add(const renderFlags& shader,
		         gameObject::ptr obj,
		         const glm::mat4& trans = glm::mat4(1),
		         bool inverted = false);

		// TODO:
		//void add(multiRenderQueue& other);

		void clear(void);
};

void updateLights(renderContext::ptr rctx, renderQueue& lights);
void updateReflections(renderContext::ptr rctx, renderQueue& refs);
void updateReflectionProbe(renderContext::ptr rctx, renderQueue& que, camera::ptr cam);
void sortQueue(renderQueue& queue, camera::ptr cam);
void cullQueue(renderQueue& queue, camera::ptr cam, unsigned width, unsigned height, float lightext);
void sortQueue(multiRenderQueue& queue, camera::ptr cam);
void cullQueue(multiRenderQueue& queue, camera::ptr cam, unsigned width, unsigned height, float lightext);
void batchQueue(renderQueue& queue);

void shaderSync(Program::ptr program, renderContext::ptr rctx, renderQueue& que);

// TODO: rename flushQueue
unsigned flush(renderQueue& que,
               camera::ptr cam,
               renderFramebuffer::ptr fb,
               renderContext::ptr rctx,
               const renderFlags& flags);

unsigned flush(multiRenderQueue& que,
               camera::ptr cam,
               renderFramebuffer::ptr fb,
               renderContext::ptr rctx);

// TODO: this for multiRenderQueue
unsigned flush(renderQueue& que,
               camera::ptr cam,
               unsigned width,
               unsigned height,
               renderContext::ptr rctx,
               const renderFlags& flags);

// namespace grendx
}
