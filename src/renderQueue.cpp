#include <grend/engine.hpp>
#include <grend/utility.hpp>
#include <grend/textureAtlas.hpp>
#include <math.h>

using namespace grendx;

void renderQueue::add(sceneNode::ptr obj,
                      uint32_t renderID,
                      glm::mat4 trans,
                      bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	const glm::mat4& adjTrans =
		// default transform is identity, so no need to waste
		// cycles on the matrix multiply
		obj->hasDefaultTransform()
			? trans
			: trans*obj->getTransformMatrix();

	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->getTransformTRS().scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	if (invcount&1) {
		inverted = !inverted;
	}

	if (obj->type == sceneNode::objType::Mesh) {
		// TODO: addMesh()
		sceneMesh::ptr mesh = std::static_pointer_cast<sceneMesh>(obj);

		if (mesh->comped_mesh) {
			glm::vec3 center = applyTransform(adjTrans);

			queueEnt<sceneMesh::ptr> entry = {
				adjTrans,
				center,
				inverted,
				mesh,
				renderID
			};

			switch (mesh->comped_mesh->blend) {
				case material::blend_mode::Blend:
					meshesBlend.push_back(entry);
					break;

				case material::blend_mode::Mask:
					meshesMasked.push_back(entry);
					break;

				case material::blend_mode::Opaque:
				default:
					meshes.push_back(entry);
					break;
			}
		}

	} else if (obj->type == sceneNode::objType::Light) {
		sceneLight::ptr light = std::static_pointer_cast<sceneLight>(obj);
		glm::vec3 center = applyTransform(adjTrans);
		lights.push_back({adjTrans, center, inverted, light});

	} else if (obj->type == sceneNode::objType::ReflectionProbe) {
		sceneReflectionProbe::ptr probe =
			std::static_pointer_cast<sceneReflectionProbe>(obj);
		glm::vec3 center = applyTransform(adjTrans);
		probes.push_back({adjTrans, center, inverted, probe});

	} else if (obj->type == sceneNode::objType::IrradianceProbe) {
		sceneIrradianceProbe::ptr probe =
			std::static_pointer_cast<sceneIrradianceProbe>(obj);
		glm::vec3 center = applyTransform(adjTrans);
		irradProbes.push_back({adjTrans, center, inverted, probe});
	}

	if (obj->type == sceneNode::objType::None
	    && obj->hasNode("skin")
	    && obj->hasNode("mesh"))
	{
		auto s = std::static_pointer_cast<sceneSkin>(obj->getNode("skin"));
		addSkinned(obj->getNode("mesh"), s, renderID, adjTrans, inverted);

	} else if (obj->type == sceneNode::objType::Particles) {
		auto p = std::static_pointer_cast<sceneParticles>(obj);
		addInstanced(obj, p, renderID, adjTrans, glm::mat4(1), inverted);

	} else if (obj->type == sceneNode::objType::BillboardParticles) {
		auto p = std::static_pointer_cast<sceneBillboardParticles>(obj);
		addBillboards(obj, p, renderID, adjTrans, inverted);

	} else {
		for (auto& [name, ptr] : obj->nodes) {
			//std::cerr << "add(): subnode " << name << std::endl;
			add(ptr, renderID, adjTrans, inverted);
		}
	}

	//std::cerr << "add(): pop" << std::endl;
}

void renderQueue::add(renderQueue& other) {
#define QUEAPPEND(SYM) \
	SYM.insert(SYM.end(), other.SYM.begin(), other.SYM.end())

	QUEAPPEND(meshes);
	QUEAPPEND(meshesBlend);
	QUEAPPEND(meshesMasked);

	//QUEAPPEND(skinnedMeshes);
	QUEAPPEND(lights);
	QUEAPPEND(probes);
	QUEAPPEND(irradProbes);
	skinnedMeshes.insert(other.skinnedMeshes.begin(), other.skinnedMeshes.end());
#undef QUEAPPEND
}

void renderQueue::addSkinned(sceneNode::ptr obj,
                             sceneSkin::ptr skin,
                             uint32_t renderID,
                             glm::mat4 trans,
                             bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	if (obj->type == sceneNode::objType::Mesh) {
		auto m = std::static_pointer_cast<sceneMesh>(obj);
		glm::vec3 center = applyTransform(trans, boxCenter(m->boundingBox));
		skinnedMeshes[skin].push_back({trans, center, inverted, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addSkinned(ptr, skin, renderID, trans, inverted);
	}
}

void renderQueue::addInstanced(sceneNode::ptr obj,
                               sceneParticles::ptr particles,
                               uint32_t renderID,
                               glm::mat4 outerTrans,
                               glm::mat4 innerTrans,
                               bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	glm::mat4 adjTrans = innerTrans*obj->getTransformMatrix();

	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->getTransformTRS().scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	inverted ^= invcount & 1;

	if (obj->type == sceneNode::objType::Mesh) {
		auto m = std::static_pointer_cast<sceneMesh>(obj);
		instancedMeshes.push_back({adjTrans, outerTrans, inverted, particles, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addInstanced(ptr, particles, renderID, outerTrans, adjTrans, inverted);
	}
}

void renderQueue::addBillboards(sceneNode::ptr obj,
                                sceneBillboardParticles::ptr particles,
                                uint32_t renderID,
                                glm::mat4 trans,
                                bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	glm::mat4 adjTrans = trans*obj->getTransformMatrix();

	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->getTransformTRS().scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	inverted ^= invcount & 1;

	if (obj->type == sceneNode::objType::Mesh) {
		auto m = std::static_pointer_cast<sceneMesh>(obj);
		billboardMeshes.push_back({adjTrans, inverted, particles, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addBillboards(ptr, particles, renderID, adjTrans, inverted);
	}
}

void grendx::updateLights(renderContext::ptr rctx,
                          renderQueue& que)
{
	// TODO: should probably apply the transform to light position
	for (auto& light : que.lights) {
		if (!light.data->casts_shadows) {
			continue;
		}

		if (light.data->lightType == sceneLight::lightTypes::Point) {
			// TODO: check against view frustum to see if this light is visible,
			//       avoid rendering shadow maps if not
			sceneLightPoint::ptr plit =
				std::static_pointer_cast<sceneLightPoint>(light.data);

			auto& shatree = rctx->atlases.shadows->tree;
			for (unsigned i = 0; i < 6; i++) {
				if (!shatree.valid(plit->shadowmap[i])) {
					// TODO: configurable size
					plit->shadowmap[i] = shatree.alloc(rctx->settings.shadowSize);
				}
			}

			drawShadowCubeMap(que, plit, light.transform, rctx);

		} else if (light.data->lightType == sceneLight::lightTypes::Spot) {
			sceneLightSpot::ptr slit =
				std::static_pointer_cast<sceneLightSpot>(light.data);

			auto& shatree = rctx->atlases.shadows->tree;
			if (!shatree.valid(slit->shadowmap)) {
				// TODO: configurable size
				slit->shadowmap = shatree.alloc(rctx->settings.shadowSize);
			}

			drawSpotlightShadow(que, slit, light.transform, rctx);

		} else if (light.data->lightType == sceneLight::lightTypes::Directional) {
			// TODO: orthoganal shadow map
		}
	}
}

void grendx::updateReflections(renderContext::ptr rctx, renderQueue& que) {
	auto& reftree = rctx->atlases.reflections->tree;
	auto& radtree = rctx->atlases.irradiance->tree;

	for (auto& probe : que.probes) {
		// allocate from reflection atlas for top level reflections
		for (unsigned i = 0; i < 6; i++) {
			if (!reftree.valid(probe.data->faces[0][i])) {
				// TODO: configurable size
				probe.data->faces[0][i] = reftree.alloc(rctx->settings.reflectionSize);
			}
		}

		// allocate from irradiance atlas for lower level (blurrier) reflections
		for (unsigned k = 1; k < 5; k++) {
			for (unsigned i = 0; i < 6; i++) {
				unsigned adj = rctx->settings.reflectionSize >> k;
				adj = adj? adj : 1;

				if (!radtree.valid(probe.data->faces[k][i])) {
					// TODO: configurable size
					probe.data->faces[k][i] = radtree.alloc(adj);
				}
			}
		}

		probe.data->have_convolved = true;
		drawReflectionProbe(que, probe.data, probe.transform, rctx);
	}

	for (auto& radprobe : que.irradProbes) {
		for (unsigned i = 0; i < 6; i++) {
			// TODO: configurable size
			if (!radtree.valid(radprobe.data->faces[i])) {
				radprobe.data->faces[i] = radtree.alloc(rctx->settings.lightProbeSize);
			}

			// TODO: coefficients

			if (!reftree.valid(radprobe.data->source->faces[0][i])) {
				radprobe.data->source->faces[0][i] =
					reftree.alloc(rctx->settings.lightProbeSize*4);
			}
		}

		radprobe.data->source->have_convolved = false;
		drawIrradianceProbe(que, radprobe.data, radprobe.transform, rctx);
	}
}

// TODO:
template <typename E>
std::vector<E> idxSort(std::vector<E>& vec, std::function<bool(unsigned, unsigned)> compare) {
	std::vector<unsigned> ret;

	ret.resize(vec.size());

	for (size_t i = 0; i < ret.size(); i++) {
		ret[i] = i;
	}

	std::sort(ret.begin(), ret.end(), compare);

	std::vector<E> foo;
	foo.resize(vec.size());

	for (size_t i = 0; i < ret.size(); i++) {
		foo[i] = vec[ret[i]];
	}

	return foo;
}

void grendx::sortQueue(renderQueue& queue, camera::ptr cam) {
	// TODO: better way to do this, in-place?
	renderQueue::MeshQ transparent;
	renderQueue::MeshQ opaque;

	transparent.reserve(queue.meshes.size());
	opaque.reserve(queue.meshes.size());

	using Ent = renderQueue::queueEnt<sceneMesh::ptr>;
	std::vector<float> distances;
	distances.resize(queue.meshes.size());

	for (size_t i = 0; i < distances.size(); i++) {
		distances[i] = glm::distance(cam->position(), queue.meshes[i].center);
	}

	queue.meshes = idxSort(queue.meshes,
		[&] (unsigned a, unsigned b) {
			return distances[a] < distances[b];
		});

	// Wait, did I need light sorting? Don't know why I have this here
	// probably for shadow map ordering? shouldn't be necessary
	/*
	std::sort(queue.lights.begin(), queue.lights.end(),
		[&] (auto& a, auto& b) {
			return glm::distance(cam->position(), a.center)
			     < glm::distance(cam->position(), b.center);
		});

	std::sort(queue.meshes.begin(), queue.meshes.end(),
		[&] (auto& a, auto& b) {
			return glm::distance(cam->position(), a.center)
			     < glm::distance(cam->position(), b.center);
		});
		*/

	// TODO: probably better to split transparent meshes out
	//       when adding to the queue, then sort individually rather than
	//       split after sorting
	for (auto& it : queue.meshes) {
		if (it.data->comped_mesh
		    && it.data->comped_mesh->blend != material::blend_mode::Opaque)
		{
			transparent.push_back(it);

		} else {
			opaque.push_back(it);
		}
	}

	std::reverse(transparent.begin(), transparent.end());

	// XXX: TODO: avoid clearing, do whole thing in-place
	queue.meshes.clear();
	queue.meshes.insert(queue.meshes.end(), opaque.begin(), opaque.end());
	queue.meshes.insert(queue.meshes.end(), transparent.begin(), transparent.end());
}

// TODO: seperate culls into functions for more granular culling
//       (particularly lights, need to trim lights before updating
//        shadow maps)
void grendx::cullQueue(renderQueue& queue,
                       camera::ptr cam,
                       unsigned width,
                       unsigned height,
                       float lightext)
{
	// TODO: reserve a vector containing indexes and cull on that,
	//       then copy indexes into the final output vector,
	//       push_back() here is disgustingly bad
	// TODO: optional OBB test after testing spheres (maybe separate function)
	// TODO: maybe multithreaded
	// TODO: occlusion culling (maybe a seperate function)
	renderQueue::MeshQ tempMeshes;
	tempMeshes.reserve(queue.meshes.size());

	for (auto it = queue.meshes.begin(); it != queue.meshes.end(); it++) {
		auto& trans = it->transform;
		auto& mesh  = it->data;
		/*
		auto obb = trans * mesh->boundingBox;

		bool visible =
			// TODO: might want just AABBs for performance, maybe an option?
			cam->boxInFrustum(obb)
			// need a compiled mesh
			&& mesh->comped_mesh
			// never need to draw fully transparent meshes
			&& (!mesh->comped_mesh->mat
			    || mesh->comped_mesh->mat->factors.opacity > 0.0001);
				*/

		auto sphere = trans * mesh->boundingSphere;

		bool visible =
			// TODO: might want just AABBs for performance, maybe an option?
			cam->sphereInFrustum(sphere)
			// need a compiled mesh
			&& mesh->comped_mesh
			// never need to draw fully transparent meshes
			&& (!mesh->comped_mesh->mat
			    || mesh->comped_mesh->mat->factors.opacity > 0.0001);

		if (visible) {
			tempMeshes.push_back(*it);
		}
	}

	queue.meshes = tempMeshes;

	for (auto& [skin, skmeshes] : queue.skinnedMeshes) {
		renderQueue::MeshQ tempSkinned;
		tempSkinned.reserve(skmeshes.size());

		for (auto it = skmeshes.begin(); it != skmeshes.end(); it++) {
			auto& trans = it->transform;
			auto& mesh  = it->data;
			auto obb = (trans) * mesh->boundingBox;

			if (cam->boxInFrustum(obb)) {
				tempSkinned.push_back(*it);
			}
		}

		skmeshes = tempSkinned;
	}

	renderQueue::LightQ tempLights;
	tempLights.reserve(queue.lights.size());

	for (auto it = queue.lights.begin(); it != queue.lights.end(); it++) {
		auto& trans = it->transform;
		auto& lit   = it->data;

		// conservative culling, keeps any lights that may possibly affect
		// what's in view, without considering direction of spotlights, shadows
		BSphere sphere = {
			.center = applyTransform(trans),
			.extent = lit->extent(lightext),
		};

		if (cam->sphereInFrustum(sphere)) {
			tempLights.push_back(*it);
		}
	}
	queue.lights = tempLights;

	std::vector<std::tuple<glm::mat4, glm::mat4, bool,
	                       sceneParticles::ptr,
						   sceneMesh::ptr>> tempInstanced;

	tempInstanced.reserve(queue.instancedMeshes.size());
	for (auto it = queue.instancedMeshes.begin();
	     it != queue.instancedMeshes.end();
	     it++)
	{
		auto& [_, trans, __, particles, ___] = *it;

		BSphere sphere = {
			.center = applyTransform(trans),
			.extent = particles->radius,
		};

		if (cam->sphereInFrustum(sphere)) {
			tempInstanced.push_back(*it);
		}
	}
	queue.instancedMeshes = tempInstanced;
}

void grendx::batchQueue(renderQueue& queue) {
	std::map<sceneMesh*, unsigned> meshCount;
	bool canBatch = false;
	// TODO: configuration option
	unsigned minbatch = 16;

	for (auto& ent : queue.meshes) {
		canBatch |= ++meshCount[ent.data.get()] >= minbatch;
	}

	if (!canBatch) {
		// no meshes meet minimum batch requirement, would be slower
		// (in principle) to batch anyway
		return;
	}

	std::map<sceneMesh::ptr, sceneParticles::ptr> batches;
	renderQueue::MeshQ tempMeshes;

	for (auto it = queue.meshes.begin(); it != queue.meshes.end(); it++) {
		auto& trans = it->transform;
		auto& mesh  = it->data;

		if (meshCount[mesh.get()] < minbatch) {
			tempMeshes.push_back(*it);
			continue;
		}

		if (batches.find(mesh) == batches.end()) {
			batches[mesh] = std::make_shared<sceneParticles>();
		}

		auto& batch = batches[mesh];

		if (batch->activeInstances < batch->maxInstances) {
			// TODO: if more than 256 meshes, add new instance buffer
			batch->positions[batch->activeInstances] = trans;
			batch->activeInstances++;
		}
	}

	for (auto& [mesh, particles] : batches) {
		particles->syncBuffer();
		queue.instancedMeshes.push_back({
			glm::mat4(1),
			glm::mat4(1),
			false,
			particles,
			mesh
		});
	}

	queue.meshes = tempMeshes;
}

void renderQueue::clear(void) {
	meshes.clear();
	meshesBlend.clear();
	meshesMasked.clear();
	skinnedMeshes.clear();
	lights.clear();
	probes.clear();
	irradProbes.clear();
	instancedMeshes.clear();
	billboardMeshes.clear();
}

static void drawMesh(const renderOptions& flags,
                     renderFramebuffer::ptr fb,
                     Program::ptr program,
                     const glm::mat4& transform,
                     bool inverted,
                     uint32_t renderID,
                     sceneMesh::ptr mesh)
{
	/*
	if (fb != nullptr && hasFlag(flags.features, renderFlags::StencilTest)) {
		// TODO: abstraction for this, maybe part of renderFramebuffer
		glStencilFunc(GL_ALWAYS, renderID, ~0);
	}
	*/

	glm::mat3 m_3x3_inv_transp =
		glm::transpose(glm::inverse(model_to_world(transform)));

	program->set("m", transform);
	program->set("m_3x3_inv_transp", m_3x3_inv_transp);

	if (!hasFlag(flags.features, renderOptions::Shadowmap)) {
		set_material(program, mesh->comped_mesh);

	/*
		// enable()/disable() cache state, no need to worry about toggling
		// too much state if queue is sorted
		if (mesh->comped_mesh->blend != material::blend_mode::Opaque)
		{
			// TODO: handle mask blends
			enable(GL_BLEND);
		} else {
			disable(GL_BLEND);
		}
	*/
	}

	// TODO: need to keep track of the model face order
	if (hasFlag(flags.features, renderOptions::CullFaces)) {
		setFaceOrder(inverted? GL_CW : GL_CCW);
	}

	bindVao(mesh->comped_mesh->vao);

	/*
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(2.0);
	enable(GL_LINE_SMOOTH);
	*/
	glDrawElements(GL_TRIANGLES,
	               mesh->comped_mesh->elements->currentSize / 4 /* sizeof uint */,
	               GL_UNSIGNED_INT, 0);
	DO_ERROR_CHECK();
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static void drawMeshInstanced(const renderOptions& flags,
                              renderFramebuffer::ptr fb,
                              Program::ptr program,
                              const glm::mat4& outerTrans,
                              const glm::mat4& innerTrans,
                              bool inverted,
                              sceneParticles::ptr particles,
                              sceneMesh::ptr mesh)
{
	/*
	if (fb != nullptr && hasFlag(flags.features, renderOptions::StencilTest)) {
		// TODO: remove this, going to move this functionality outside
		//       of the render queue
		if (fb->drawn_meshes.size() < 0xff) {
			fb->drawn_meshes.push_back(mesh);
			glStencilFunc(GL_ALWAYS, fb->drawn_meshes.size(), ~0);

		} else {
			glStencilFunc(GL_ALWAYS, 0, ~0);
		}
	}
	*/

	/*
	if (fb != nullptr && hasFlag(flags.features, renderOptions::StencilTest)) {
		// TODO: abstraction for this, maybe part of renderFramebuffer
		glStencilFunc(GL_ALWAYS, renderID, ~0);
	}
	*/

	glm::mat3 m_3x3_inv_transp =
		glm::transpose(glm::inverse(model_to_world(outerTrans)));

	program->set("outerTrans", outerTrans);
	program->set("innerTrans", innerTrans);
	program->set("m_3x3_inv_transp", m_3x3_inv_transp);

	if (!hasFlag(flags.features, renderOptions::Shadowmap)) {
		set_material(program, mesh->comped_mesh);

	/*
		// enable()/disable() cache state, no need to worry about toggling
		// too much state if queue is sorted
		if (mesh->comped_mesh->blend != material::blend_mode::Opaque) {
			// TODO: handle mask blends
			enable(GL_BLEND);
		} else {
			disable(GL_BLEND);
		}
	*/
	}

	// TODO: need to keep track of the model face order
	if (hasFlag(flags.features, renderOptions::CullFaces)) {
		setFaceOrder(inverted? GL_CW : GL_CCW);
	}

	bindVao(mesh->comped_mesh->vao);

#if GLSL_VERSION >= 140
	particles->syncBuffer();
	program->setUniformBlock("instanceTransforms", particles->ubuffer,
	                         UBO_INSTANCE_TRANSFORMS);
	glDrawElementsInstanced(
		GL_TRIANGLES,
		mesh->comped_mesh->elements->currentSize / 4 /* sizeof uint */,
		GL_UNSIGNED_INT, 0, particles->activeInstances);
	DO_ERROR_CHECK();

#else
	for (unsigned i = 0; i < particles->activeInstances; i++) {
		// TODO: fallback
	}
#endif
}

static void drawBillboards(const renderOptions& flags,
                           renderFramebuffer::ptr fb,
                           Program::ptr program,
                           const glm::mat4& transform,
                           bool inverted,
                           sceneBillboardParticles::ptr particles,
                           sceneMesh::ptr mesh)
{
	if (!hasFlag(flags.features, renderOptions::Shadowmap)) {
		set_material(program, mesh->comped_mesh);

	/*
		// enable()/disable() cache state, no need to worry about toggling
		// too much state if queue is sorted
		if (mesh->comped_mesh->blend != material::blend_mode::Opaque) {
			// TODO: handle mask blends
			enable(GL_BLEND);
		} else {
			disable(GL_BLEND);
		}
	*/
	}

	glm::mat3 m_3x3_inv_transp =
		glm::transpose(glm::inverse(model_to_world(transform)));

	program->set("m", transform);
	program->set("m_3x3_inv_transp", m_3x3_inv_transp);

	//enable(GL_BLEND);

	if (hasFlag(flags.features, renderOptions::CullFaces)) {
		// TODO: face order
		setFaceOrder(inverted? GL_CW : GL_CCW);
	}

	bindVao(mesh->comped_mesh->vao);

#if GLSL_VERSION >= 140
	//SDL_Log("Drawing billboard mesh... %u instances", particles->activeInstances);
	particles->syncBuffer();
	program->setUniformBlock("billboardPositions", particles->ubuffer,
	                         UBO_INSTANCE_TRANSFORMS);
	glDrawElementsInstanced(
		GL_TRIANGLES,
		mesh->comped_mesh->elements->currentSize / 4 /* sizeof uint */,
		GL_UNSIGNED_INT, 0, particles->activeInstances);
	DO_ERROR_CHECK();

#else
	for (unsigned i = 0; i < particles->activeInstances; i++) {
		// TODO: fallback
	}
#endif
}


unsigned grendx::flush(renderQueue& que,
                       camera::ptr cam,
                       unsigned width,
                       unsigned height,
                       renderContext::ptr rctx,
                       const renderFlags& flags,
                       const renderOptions& options)
{
	// TODO: deduplicate code
	unsigned drawnMeshes = 0;

	disable(GL_BLEND);

	if (hasFlag(options.features, renderOptions::CullFaces)) {
		enable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
	}

	if (hasFlag(options.features, renderOptions::StencilTest)) {
		enable(GL_STENCIL_TEST);
		//glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilOp(options.stencilOpToGL(options.stencilFail),
		            options.stencilOpToGL(options.depthFail),
		            options.stencilOpToGL(options.depthPass));
	}

	if (hasFlag(options.features, renderOptions::DepthTest)) {
		enable(GL_DEPTH_TEST);
	} else {
		disable(GL_DEPTH_TEST);
	}

	bool depthMask = hasFlag(options.features, renderOptions::DepthMask);
	glDepthMask(depthMask? GL_TRUE : GL_FALSE);

	cam->setViewport(width, height);
	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform();
	glm::mat4 v_inv = glm::inverse(view);

	flags.skinnedShader->bind();
	flags.skinnedShader->set("v", view);
	flags.skinnedShader->set("p", projection);
	flags.skinnedShader->set("v_inv", v_inv);
	flags.skinnedShader->set("cameraPosition", cam->position());

	for (auto& [skin, drawinfo] : que.skinnedMeshes) {
		for (auto& mesh : drawinfo) {
			skin->sync(flags.skinnedShader);

			if (!hasFlag(options.features, renderOptions::Shadowmap)) {
				rctx->setIrradianceProbe(
					que.nearest_irradiance_probe(mesh.center),
					flags.skinnedShader);
			}

			drawMesh(options, nullptr, flags.skinnedShader,
			         mesh.transform, mesh.inverted, mesh.renderID, mesh.data);
		}

		// TODO: check whether this skin is already synced
		DO_ERROR_CHECK();
	}

	flags.mainShader->bind();
	flags.mainShader->set("v", view);
	flags.mainShader->set("p", projection);
	flags.mainShader->set("v_inv", v_inv);
	flags.mainShader->set("cameraPosition", cam->position());

	//for (auto& [transform, inverted, mesh] : meshes) {
	for (auto& mesh : que.meshes) {
		if (!hasFlag(options.features, renderOptions::Shadowmap)) {
			rctx->setIrradianceProbe(
				que.nearest_irradiance_probe(mesh.center),
				flags.mainShader);
		}

		drawMesh(options, nullptr, flags.mainShader, mesh.transform,
		         mesh.inverted, mesh.renderID, mesh.data);
		drawnMeshes++;
	}

	/*
	meshes.clear();
	lights.clear();
	probes.clear();
	skinnedMeshes.clear();
	instancedMeshes.clear();
	billboardMeshes.clear();
	*/
	que.clear();

	return drawnMeshes;
}

unsigned grendx::flush(renderQueue& que,
                       camera::ptr cam,
                       renderFramebuffer::ptr fb,
                       renderContext::ptr rctx,
                       const renderFlags& flags,
                       const renderOptions& options)
{
	DO_ERROR_CHECK();
	unsigned drawnMeshes = 0;

	//if (flags.sort) { sort(); }

	disable(GL_BLEND);
	if (hasFlag(options.features, renderOptions::CullFaces)) {
		enable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
	}

	if (hasFlag(options.features, renderOptions::StencilTest)) {
		enable(GL_STENCIL_TEST);

		//glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilOp(options.stencilOpToGL(options.stencilFail),
		            options.stencilOpToGL(options.depthFail),
		            options.stencilOpToGL(options.depthPass));
	}

	if (hasFlag(options.features, renderOptions::DepthTest)) {
		enable(GL_DEPTH_TEST);
	} else {
		disable(GL_DEPTH_TEST);
	}

	bool depthMask = hasFlag(options.features, renderOptions::DepthMask);
	glDepthMask(depthMask? GL_TRUE : GL_FALSE);

	DO_ERROR_CHECK();
	assert(fb != nullptr);
	//fb->framebuffer->bind();
	fb->bind();

	disable(GL_SCISSOR_TEST);
	glViewport(0, 0, fb->width, fb->height);
	glScissor(0, 0, fb->width, fb->height);

	cam->setViewport(fb->width/rctx->settings.scaleX, fb->height/rctx->settings.scaleY);
	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform();
	glm::mat4 v_inv = glm::inverse(view);

	flags.skinnedShader->bind();
	flags.skinnedShader->set("v", view);
	flags.skinnedShader->set("p", projection);
	flags.skinnedShader->set("v_inv", v_inv);
	flags.skinnedShader->set("cameraPosition", cam->position());
	shaderSync(flags.skinnedShader, rctx, que);
	sceneSkin::ptr lastskin = nullptr;

	for (auto& [skin, drawinfo] : que.skinnedMeshes) {
		for (auto& mesh : drawinfo) {
			skin->sync(flags.skinnedShader);

			if (!hasFlag(options.features, renderOptions::Shadowmap)) {
				rctx->setIrradianceProbe(
					que.nearest_irradiance_probe(mesh.center),
					flags.skinnedShader);
			}

			drawMesh(options, fb, flags.skinnedShader, mesh.transform,
			         mesh.inverted, mesh.renderID, mesh.data);
			drawnMeshes++;
		}

		// TODO: check whether this skin is already synced
		DO_ERROR_CHECK();
	}

	flags.mainShader->bind();
	flags.mainShader->set("v", view);
	flags.mainShader->set("p", projection);
	flags.mainShader->set("v_inv", v_inv);
	flags.mainShader->set("cameraPosition", cam->position());
	shaderSync(flags.mainShader, rctx, que);

	//for (auto& [transform, inverted, mesh] : meshes) {
	for (auto& mesh : que.meshes) {
		if (!hasFlag(options.features, renderOptions::Shadowmap)) {
			rctx->setIrradianceProbe(
				que.nearest_irradiance_probe(mesh.center),
				flags.mainShader);
		}

		drawMesh(options, fb, flags.mainShader, mesh.transform,
		         mesh.inverted, mesh.renderID, mesh.data);
		drawnMeshes++;
	}

	flags.instancedShader->bind();
	flags.instancedShader->set("v", view);
	flags.instancedShader->set("p", projection);
	flags.instancedShader->set("v_inv", v_inv);
	flags.instancedShader->set("cameraPosition", cam->position());
	shaderSync(flags.instancedShader, rctx, que);

	for (auto& [innerTrans, outerTrans, inverted, particleSystem, mesh] : que.instancedMeshes) {
		if (!hasFlag(options.features, renderOptions::Shadowmap)) {
			rctx->setIrradianceProbe(
				que.nearest_irradiance_probe(applyTransform(outerTrans)),
				flags.instancedShader);
		}

		drawMeshInstanced(options, fb, flags.instancedShader,
		                  innerTrans, outerTrans, inverted,
		                  particleSystem, mesh);
		drawnMeshes += particleSystem->activeInstances;
	}

	glDepthMask(GL_FALSE);

	flags.billboardShader->bind();
	DO_ERROR_CHECK();
	flags.billboardShader->set("v", view);
	flags.billboardShader->set("p", projection);
	flags.billboardShader->set("v_inv", v_inv);
	flags.billboardShader->set("cameraPosition", cam->position());
	DO_ERROR_CHECK();
	shaderSync(flags.billboardShader, rctx, que);
	DO_ERROR_CHECK();

	for (auto& [transform, inverted, particleSystem, mesh] : que.billboardMeshes) {
		if (!hasFlag(options.features, renderOptions::Shadowmap)) {
			rctx->setIrradianceProbe(
				que.nearest_irradiance_probe(applyTransform(transform)),
				flags.billboardShader);
		}

		drawBillboards(options, fb, flags.billboardShader,
		               transform, inverted, particleSystem, mesh);
		drawnMeshes += particleSystem->activeInstances;
		DO_ERROR_CHECK();
	}

	// TODO: clear()
	/*
	meshes.clear();
	lights.clear();
	probes.clear();
	skinnedMeshes.clear();
	instancedMeshes.clear();
	billboardMeshes.clear();
	*/
	que.clear();

	return drawnMeshes;
}

typedef std::vector<std::pair<glm::mat4&, sceneLightPoint::ptr>>       pointList;
typedef std::vector<std::pair<glm::mat4&, sceneLightSpot::ptr>>        spotList;
typedef std::vector<std::pair<glm::mat4&, sceneLightDirectional::ptr>> dirList;
typedef std::tuple<pointList, spotList, dirList> lightLists;

static void syncPlainUniforms(Program::ptr program,
	                          renderContext::ptr rctx,
	                          pointList& points,
	                          spotList& spots,
	                          dirList&  directionals)
{
	size_t pactive = min((size_t)MAX_LIGHTS, points.size());
	size_t sactive = min((size_t)MAX_LIGHTS, spots.size());
	size_t dactive = min((size_t)MAX_LIGHTS, directionals.size());

	program->set("active_point_lights",       (GLint)pactive);
	program->set("active_spot_lights",        (GLint)sactive);
	program->set("active_directional_lights", (GLint)dactive);

	SDL_Log(">>> syncing plain uniforms: %lu point, %lu spot, %lu directional",
		pactive, sactive, dactive);

	for (size_t i = 0; i < pactive; i++) {
		//sceneLightPoint::ptr light = points[i];
		auto& [trans, light] = points[i];
		glm::vec3 pos = applyTransform(trans, light->getTransformTRS().position);

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "point_lights[" + std::to_string(i) + "]";

		program->set(locstr + ".position",      glm::vec4(pos, 1.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".diffuse",       glm::vec4(light->diffuse));
		DO_ERROR_CHECK();
		program->set(locstr + ".intensity",     light->intensity);
		DO_ERROR_CHECK();
		program->set(locstr + ".radius",        light->radius);
		DO_ERROR_CHECK();
		program->set(locstr + ".casts_shadows", light->casts_shadows);
		DO_ERROR_CHECK();

		if (light->casts_shadows) {
			for (unsigned k = 0; k < 6; k++) {
				std::string sloc = locstr + ".shadowmap[" + std::to_string(k) + "]";
				auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap[k]);
				program->set(sloc, glm::vec4(vec, 0.0));
				DO_ERROR_CHECK();
			}
		}
	}

	for (size_t i = 0; i < sactive; i++) {
		//sceneLightSpot::ptr light = spots[i];
		auto& [trans, light] = spots[i];

		// TODO: possibly have a seperate version of this that uses UBOs
		//       on higher versions of opengl
		TRS transform = light->getTransformTRS();
		std::string locstr = "spot_lights[" + std::to_string(i) + "]";
		// light points toward -Z by default
		glm::vec3 rotvec = glm::mat3_cast(transform.rotation) * glm::vec3(0, 0, -1);

		program->set(locstr + ".position",      glm::vec4(transform.position, 1.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".diffuse",       glm::vec4(light->diffuse));
		DO_ERROR_CHECK();
		program->set(locstr + ".direction",     glm::vec4(rotvec, 0.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".intensity",     light->intensity);
		DO_ERROR_CHECK();
		program->set(locstr + ".radius",        light->radius);
		DO_ERROR_CHECK();
		program->set(locstr + ".angle",         light->angle);
		DO_ERROR_CHECK();
		program->set(locstr + ".casts_shadows", light->casts_shadows);
		DO_ERROR_CHECK();

		if (light->casts_shadows) {
			std::string smap  = locstr + ".shadowmap";
			std::string sproj = locstr + ".shadowproj";

			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
			program->set(smap, glm::vec4(vec, 0.0));
			program->set(sproj, light->shadowproj);
			DO_ERROR_CHECK();
		}
	}

	for (size_t i = 0; i < dactive; i++) {
		//sceneLightDirectional::ptr light = directionals[i];
		auto& [trans, light] = directionals[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		TRS transform = light->getTransformTRS();
		std::string locstr = "directional_lights[" + std::to_string(i) + "]";
		// light points toward +X by default
		// TODO: -Z to match spotlights
		glm::vec3 rotvec = glm::mat3_cast(transform.rotation) * glm::vec3(1, 0, 0);

		// position is ignored in the shader, but might as well set it anyway
		// TODO: probably remove position member from directional lights
		program->set(locstr + ".position",      glm::vec4(transform.position, 0.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".diffuse",       glm::vec4(light->diffuse));
		DO_ERROR_CHECK();
		program->set(locstr + ".direction",     glm::vec4(rotvec, 0.0));
		DO_ERROR_CHECK();
		program->set(locstr + ".intensity",     light->intensity);
		DO_ERROR_CHECK();
		program->set(locstr + ".casts_shadows", light->casts_shadows);
		DO_ERROR_CHECK();

		if (light->casts_shadows) {
			std::string smap  = locstr + ".shadowmap";
			std::string sproj = locstr + ".shadowproj";

			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
			program->set(smap, glm::vec4(vec, 0.0));
			program->set(sproj, light->shadowproj);
			DO_ERROR_CHECK();
		}
	}
}

static inline float rectScale(float R, float d) {
	return 1.f / cos(d/R);
}

void grendx::buildTilemap(renderQueue::LightQ& queue,
                          camera::ptr cam,
                          renderContext::ptr rctx)
{
	switch (rctx->lightingMode) {
		case renderContext::lightingModes::Clustered:
			// TODO:
			break;

		case renderContext::lightingModes::Tiled:
			buildTilemapTiled(queue, cam, rctx);
			break;

		case renderContext::lightingModes::PlainArray:
		default:
			// nothing to do
			break;
	}
}

// TODO: possibly move this to it's own file
#include <grend/plane.hpp>
void grendx::buildTilemapTiled(renderQueue::LightQ& queue,
                               camera::ptr cam,
                               renderContext::ptr rctx)
{
	lights_std140&      lightbuf = rctx->lightBufferCtx;
	light_tiles_std140& pointbuf = rctx->pointTilesCtx;
	light_tiles_std140& spotbuf  = rctx->spotTilesCtx;

	float rx = glm::radians(cam->fovx());
	float ry = glm::radians(cam->fovy());

	size_t activePoints = 0;
	size_t activeSpots = 0;
	size_t activeDirs = 0;

	// TODO: put arrays in one big struct so that this can be just one memset()
	memset(&lightbuf.upoint_lights, 0, sizeof(lightbuf.upoint_lights));
	memset(&lightbuf.uspot_lights, 0, sizeof(lightbuf.uspot_lights));
	memset(&lightbuf.udirectional_lights, 0, sizeof(lightbuf.udirectional_lights));
	memset(&pointbuf, 0, sizeof(pointbuf));
	memset(&spotbuf,  0, sizeof(spotbuf));

	plane planes[16*9*6];
	glm::vec3 nearpos = cam->position() + cam->direction()*cam->near();
	glm::vec3 cpos = cam->position();

	unsigned x = 0;
	for (float tx = rx/2.0; x < 16; tx -= rx/16.0, x++) {
		unsigned y = 0;
		for (float ty = ry/2.0; y < 9; ty -= ry/9.0, y++) {
			unsigned cluster = x + y*16;
			plane *p = planes + cluster*6;

			// left
			float txp = tx - rx/16.f;
			p[0].n = glm::rotate(cam->right(), txp/rectScale(rx, txp), cam->up());
			p[0].d = -glm::dot(p[0].n, cpos);

			// right
			p[1].n = -glm::rotate(cam->right(), tx/rectScale(rx, tx), cam->up());
			p[1].d = -glm::dot(p[1].n, cpos);

			// bottom
			p[2].n = glm::rotate(cam->up(), ty/rectScale(ry, ty), cam->right());
			p[2].d = -glm::dot(p[2].n, cpos);

			// top
			float typ = ty - ry/9.f;
			p[3].n = -glm::rotate(cam->up(), typ/rectScale(ry, typ), cam->right());
			p[3].d = -glm::dot(p[3].n, cpos);

			// near
			p[4].n = cam->direction();
			p[4].d = -glm::dot(p[4].n, nearpos);

			// far
			p[5].n = -cam->direction();
			p[5].d = -glm::dot(p[5].n, cam->position() + cam->direction()*cam->far());
		}
	}

	// TODO: parallelism
	auto buildTiles = [&] (sceneLight::ptr lit, glm::mat4& trans, unsigned idx) {
		glm::vec3 lightpos = applyTransform(trans);
		float ext = lit->extent(rctx->lightThreshold);

		for (unsigned cluster = 0; cluster < 16*9; cluster++) {
			unsigned in = 0;
			plane *p = planes + cluster*6;

			for (unsigned i = 0; i < 6; i++) {
				in += p[i].inPlane(lightpos, ext);
			}

			if (in == 6) {
				unsigned clusidx = MAX_LIGHTS * cluster;

				if (lit->lightType == sceneLight::lightTypes::Point) {
					unsigned cur  = pointbuf.indexes[clusidx];
					unsigned next = cur + 1;

					if (next < MAX_LIGHTS) {
						pointbuf.indexes[clusidx]        = next;
						pointbuf.indexes[clusidx + next] = idx;
					}

				} else if (lit->lightType == sceneLight::lightTypes::Spot) {
					unsigned cur  = spotbuf.indexes[clusidx];
					unsigned next = cur + 1;

					if (next < MAX_LIGHTS) {
						spotbuf.indexes[clusidx]        = next;
						spotbuf.indexes[clusidx + next] = idx;
					}
				}
			}
		}
	};

	//for (auto& [trans, _, lit] : queue.lights) {
	for (auto& lit : queue) {
		if (activePoints < MAX_POINT_LIGHT_OBJECTS_TILED &&
		    lit.data->lightType == sceneLight::lightTypes::Point)
		{
			sceneLightPoint::ptr plit =
				std::static_pointer_cast<sceneLightPoint>(lit.data);

			packLight(plit,
					  lightbuf.upoint_lights + activePoints,
					  rctx, lit.transform);
			buildTiles(lit.data, lit.transform, activePoints);
			activePoints++;

		} else if (activeSpots < MAX_SPOT_LIGHT_OBJECTS_TILED
		           && lit.data->lightType == sceneLight::lightTypes::Spot)
		{
			sceneLightSpot::ptr slit =
				std::static_pointer_cast<sceneLightSpot>(lit.data);

			packLight(slit,
					  lightbuf.uspot_lights + activeSpots,
					  rctx, lit.transform);
			buildTiles(lit.data, lit.transform, activeSpots);
			activeSpots++;

		} else if (activeDirs < MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED
		          && lit.data->lightType == sceneLight::lightTypes::Directional)
		{
			sceneLightDirectional::ptr dlit =
				std::static_pointer_cast<sceneLightDirectional>(lit.data);

			packLight(dlit,
					  lightbuf.udirectional_lights + activeDirs,
					  rctx, lit.transform);
			activeDirs++;
		}
	}

	lightbuf.uactive_point_lights       = activePoints;
	lightbuf.uactive_spot_lights        = activeSpots;
	lightbuf.uactive_directional_lights = activeDirs;

	/*
	SDL_Log("updating buffers: lights: %u, points: %u, spots: %u",
	        sizeof(lightbuf), sizeof(pointbuf), sizeof(spotbuf));
			*/
	rctx->lightBuffer->update(&lightbuf, 0, sizeof(lightbuf));
	rctx->pointTiles->update(&pointbuf,  0, sizeof(pointbuf));
	rctx->spotTiles->update(&spotbuf,    0, sizeof(spotbuf));
}

void grendx::updateReflectionProbe(renderContext::ptr rctx,
                                   renderQueue& que,
                                   camera::ptr cam)
{
	auto refprobe = que.nearest_reflection_probe(cam->position());

	// TODO: set the real transform
	glm::mat4 xxx(1);
	packRefprobe(refprobe, &rctx->lightBufferCtx, rctx, xxx);
	//rctx->lightBuffer->update(&rctx->lightBufferCtx, 16, 528);
	rctx->lightBuffer->update(&rctx->lightBufferCtx, 0, sizeof(rctx->lightBufferCtx));
}

void grendx::shaderSync(Program::ptr program,
                        renderContext::ptr rctx,
                        renderQueue& que)
{
	if (rctx->lightingMode == renderContext::lightingModes::Clustered) {
		// TODO: implement
		// TODO: function to set mode
		SDL_Log("TODO: clustered light array");
	}

	else if (rctx->lightingMode == renderContext::lightingModes::Tiled) {
		program->setUniformBlock("lights", rctx->lightBuffer, UBO_LIGHT_INFO);
#if !defined(USE_SINGLE_UBO)
		program->setUniformBlock("point_light_tiles", rctx->pointTiles,
								 UBO_POINT_LIGHT_TILES);
		program->setUniformBlock("spot_light_tiles", rctx->spotTiles,
								 UBO_SPOT_LIGHT_TILES);
#endif
		DO_ERROR_CHECK();
	}

	else if (rctx->lightingMode == renderContext::lightingModes::PlainArray) {
		pointList point_lights;
		spotList  spot_lights;
		dirList   directional_lights;

		//for (auto& [trans, _, light] : lights) {
		for (auto& light : que.lights) {
			switch (light.data->lightType) {
				case sceneLight::lightTypes::Point:
					{
						sceneLightPoint::ptr plit =
							std::static_pointer_cast<sceneLightPoint>(light.data);
						point_lights.push_back({light.transform, plit});
					}
					break;

				case sceneLight::lightTypes::Spot:
					{
						sceneLightSpot::ptr slit =
							std::static_pointer_cast<sceneLightSpot>(light.data);
						spot_lights.push_back({light.transform, slit});
					}
					break;

				case sceneLight::lightTypes::Directional:
					{
						sceneLightDirectional::ptr dlit =
							std::static_pointer_cast<sceneLightDirectional>(light.data);
						directional_lights.push_back({light.transform, dlit});
					}
					break;

				default:
					break;
			}
		}

		syncPlainUniforms(program, rctx, point_lights,
						  spot_lights, directional_lights);

		//set_reflection_probe(refprobe, program, rctx->atlases);
		DO_ERROR_CHECK();
	}

	program->set("renderWidth",  (float)rctx->framebuffer->width);
	program->set("renderHeight", (float)rctx->framebuffer->height);
	program->set("lightThreshold", rctx->lightThreshold);

	glActiveTexture(TEX_GL_REFLECTIONS);
	rctx->atlases.reflections->color_tex->bind();
	program->set("reflection_atlas", TEXU_REFLECTIONS);

	glActiveTexture(TEX_GL_SHADOWS);
	rctx->atlases.shadows->depth_tex->bind();
	program->set("shadowmap_atlas", TEXU_SHADOWS);
	DO_ERROR_CHECK();

	glActiveTexture(TEX_GL_IRRADIANCE);
	rctx->atlases.irradiance->color_tex->bind();
	program->set("irradiance_atlas", TEXU_IRRADIANCE);
	DO_ERROR_CHECK();
}

sceneReflectionProbe::ptr renderQueue::nearest_reflection_probe(glm::vec3 pos) {
	sceneReflectionProbe::ptr ret = nullptr;
	float mindist = HUGE_VALF;

	// TODO: optimize this, O(N) is meh
	for (auto& p : probes) {
		float dist = glm::distance(pos, p.data->getTransformTRS().position);
		if (!ret || dist < mindist) {
			mindist = dist;
			ret = p.data;
		}
	}

	return ret;
}

sceneIrradianceProbe::ptr renderQueue::nearest_irradiance_probe(glm::vec3 pos) {
	sceneIrradianceProbe::ptr ret = nullptr;
	float mindist = HUGE_VALF;

	// TODO: optimize this, O(N) is meh
	for (auto& p : irradProbes) {
		float dist = glm::distance(pos, p.data->getTransformTRS().position);
		if (!ret || dist < mindist) {
			mindist = dist;
			ret = p.data;
		}
	}

	return ret;
}
