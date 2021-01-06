#include <grend/engine.hpp>
#include <grend/utility.hpp>
#include <grend/textureAtlas.hpp>
#include <math.h>

using namespace grendx;

void renderQueue::add(gameObject::ptr obj,
                      float animTime,
                      glm::mat4 trans,
                      bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	glm::mat4 adjTrans = trans*obj->getTransform(animTime);
	//glm::mat4 adjTrans = obj->getTransform();

	//std::cerr << "add(): push" << std::endl;
	
	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->transform.scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	if (invcount&1) {
		inverted = !inverted;
	}

	if (obj->type == gameObject::objType::Mesh) {
		gameMesh::ptr mesh = std::dynamic_pointer_cast<gameMesh>(obj);
		meshes.push_back({adjTrans, inverted, mesh});

	} else if (obj->type == gameObject::objType::Light) {
		gameLight::ptr light = std::dynamic_pointer_cast<gameLight>(obj);
		lights.push_back({adjTrans, inverted, light});

	} else if (obj->type == gameObject::objType::ReflectionProbe) {
		gameReflectionProbe::ptr probe =
			std::dynamic_pointer_cast<gameReflectionProbe>(obj);
		probes.push_back({adjTrans, inverted, probe});

	} else if (obj->type == gameObject::objType::IrradianceProbe) {
		gameIrradianceProbe::ptr probe =
			std::dynamic_pointer_cast<gameIrradianceProbe>(obj);
		irradProbes.push_back({adjTrans, inverted, probe});
	}

	if (obj->type == gameObject::objType::None
	    && obj->hasNode("skin")
	    && obj->hasNode("mesh"))
	{
		auto s = std::dynamic_pointer_cast<gameSkin>(obj->getNode("skin"));
		addSkinned(obj->getNode("mesh"), s, animTime, adjTrans, inverted);

	} else if (obj->type == gameObject::objType::Particles) {
		auto p = std::dynamic_pointer_cast<gameParticles>(obj);
		addInstanced(obj, p, adjTrans, inverted);

	} else {
		for (auto& [name, ptr] : obj->nodes) {
			//std::cerr << "add(): subnode " << name << std::endl;
			add(ptr, animTime, adjTrans, inverted);
		}
	}

	//std::cerr << "add(): pop" << std::endl;
}

void renderQueue::addSkinned(gameObject::ptr obj,
                             gameSkin::ptr skin,
                             float animTime,
                             glm::mat4 trans,
                             bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	if (obj->type == gameObject::objType::Mesh) {
		auto m = std::dynamic_pointer_cast<gameMesh>(obj);
		skinnedMeshes[skin].push_back({trans, inverted, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addSkinned(ptr, skin, animTime, trans, inverted);
	}
}

void renderQueue::addInstanced(gameObject::ptr obj,
                               gameParticles::ptr particles,
                               glm::mat4 trans,
                               bool inverted)
{
	if (obj == nullptr || !obj->visible) {
		return;
	}

	glm::mat4 adjTrans = trans*obj->getTransform(0);

	unsigned invcount = 0;
	for (unsigned i = 0; i < 3; i++)
		invcount += obj->transform.scale[i] < 0;

	// only want to invert face order if flipped an odd number of times
	inverted ^= invcount & 1;

	if (obj->type == gameObject::objType::Mesh) {
		auto m = std::dynamic_pointer_cast<gameMesh>(obj);
		instancedMeshes.push_back({adjTrans, inverted, particles, m});
	}

	for (auto& [name, ptr] : obj->nodes) {
		addInstanced(ptr, particles, adjTrans, inverted);
	}
}

void renderQueue::updateLights(renderContext::ptr rctx) {
	// TODO: should probably apply the transform to light position
	for (auto& [trans, __, light] : lights) {
		if (!light->casts_shadows) {
			continue;
		}

		if (light->lightType == gameLight::lightTypes::Point) {
			// TODO: check against view frustum to see if this light is visible,
			//       avoid rendering shadow maps if not
			gameLightPoint::ptr plit =
				std::dynamic_pointer_cast<gameLightPoint>(light);

			auto& shatree = rctx->atlases.shadows->tree;
			for (unsigned i = 0; i < 6; i++) {
				if (!shatree.valid(plit->shadowmap[i])) {
					// TODO: configurable size
					plit->shadowmap[i] = shatree.alloc(256);
				}
			}

			drawShadowCubeMap(*this, plit, trans, rctx);

		} else if (light->lightType == gameLight::lightTypes::Spot) {
			// TODO: shadow map

		} else if (light->lightType == gameLight::lightTypes::Directional) {
			// TODO: orthoganal shadow map
		}
	}
}

void
renderQueue::updateReflections(renderContext::ptr rctx) {
	for (auto& [trans, __, probe] : probes) {
		auto& reftree = rctx->atlases.reflections->tree;
		auto& radtree = rctx->atlases.irradiance->tree;

		// allocate from reflection atlas for top level reflections
		for (unsigned i = 0; i < 6; i++) {
			if (!reftree.valid(probe->faces[0][i])) {
				// TODO: configurable size
				probe->faces[0][i] = reftree.alloc(128);
			}
		}

		// allocate from irradiance atlas for lower level (blurrier) reflections
		for (unsigned k = 1; k < 5; k++) {
			for (unsigned i = 0; i < 6; i++) {
				if (!radtree.valid(probe->faces[k][i])) {
					// TODO: configurable size
					probe->faces[k][i] = radtree.alloc(128 >> k);
				}
			}
		}

		probe->have_convolved = true;
		drawReflectionProbe(*this, probe, trans, rctx);
	}

	for (auto& [trans, __, radprobe] : irradProbes) {
		auto& radtree = rctx->atlases.irradiance->tree;
		auto& reftree = rctx->atlases.reflections->tree;

		for (unsigned i = 0; i < 6; i++) {
			// TODO: configurable size
			if (!radtree.valid(radprobe->faces[i])) {
				radprobe->faces[i] = radtree.alloc(16);
			}

			// TODO: coefficients

			if (!reftree.valid(radprobe->source->faces[0][i])) {
				radprobe->source->faces[0][i] = reftree.alloc(64);
			}
		}

		radprobe->source->have_convolved = false;
		drawIrradianceProbe(*this, radprobe, trans, rctx);
	}
}

void renderQueue::sort(void) {
	typedef std::tuple<glm::mat4, bool, gameMesh::ptr>  draw_ent;
	typedef std::tuple<glm::mat4, bool, gameLight::ptr> light_ent;

	// TODO: better way to do this, in-place?
	std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> transparent;
	std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> opaque;

	std::sort(lights.begin(), lights.end(),
		[&] (light_ent& a, light_ent& b) {
			// TODO: why not just define a struct
			auto& [a_trans, _,  a_mesh] = a;
			auto& [b_trans, __, b_mesh] = b;

			glm::vec3 va = applyTransform(a_trans);
			glm::vec3 vb = applyTransform(b_trans);

			return glm::distance(cam->position(), va)
				< glm::distance(cam->position(), vb);
		});

	std::sort(meshes.begin(), meshes.end(),
		[&] (draw_ent& a, draw_ent& b) {
			// TODO: why not just define a struct
			auto& [a_trans, _,  a_mesh] = a;
			auto& [b_trans, __, b_mesh] = b;

			glm::vec3 va = applyTransform(a_trans, boxCenter(a_mesh->boundingBox));
			glm::vec3 vb = applyTransform(b_trans, boxCenter(b_mesh->boundingBox));

			return glm::distance(cam->position(), va)
				< glm::distance(cam->position(), vb);
		});

	for (auto& it : meshes) {
		auto [transform, inverted, mesh] = it;

		if (mesh->comped_mesh->factors.blend != material::blend_mode::Opaque) {
			transparent.push_back(it);
		} else {
			opaque.push_back(it);
		}
	}

	std::reverse(transparent.begin(), transparent.end());

	// XXX: TODO: avoid clearing, do whole thing in-place
	meshes.clear();
	meshes.insert(meshes.end(), opaque.begin(), opaque.end());
	meshes.insert(meshes.end(), transparent.begin(), transparent.end());
}

void renderQueue::cull(unsigned width, unsigned height, float lightext) {
	glm::mat4 view = cam->viewTransform();

	// XXX: need vector for sorting, deleting elements from vector for culling
	//      would make this quadratic in the worst case... copying isn't
	//      ideal, but should be fast enough, still linear time complexity
	std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> tempMeshes;

	for (auto it = meshes.begin(); it != meshes.end(); it++) {
		auto& [trans, _, mesh] = *it;
		auto obb = trans * mesh->boundingBox;

		if (cam->boxInFrustum(obb)) {
			tempMeshes.push_back(*it);
		}
	}

	meshes = tempMeshes;

	for (auto& [skin, skmeshes] : skinnedMeshes) {
		std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> tempSkinned;

		for (auto it = skmeshes.begin(); it != skmeshes.end(); it++) {
			auto& [trans, _, mesh] = *it;
			auto obb = (trans) * mesh->boundingBox;

			if (cam->boxInFrustum(obb)) {
				tempSkinned.push_back(*it);
			}
		}

		skmeshes = tempSkinned;
	}

	std::vector<std::tuple<glm::mat4, bool, gameLight::ptr>> tempLights;
	for (auto it = lights.begin(); it != lights.end(); it++) {
		auto& [trans, _, lit] = *it;

		// conservative culling, keeps any lights that may possibly affect
		// what's in view, without considering direction of spotlights, shadows
		if (cam->sphereInFrustum(applyTransform(trans), lit->extent(lightext))) {
			tempLights.push_back(*it);
		}
	}
	lights = tempLights;

	std::vector<std::tuple<glm::mat4, bool, gameParticles::ptr,
						   gameMesh::ptr>> tempInstanced;
	for (auto it = instancedMeshes.begin(); it != instancedMeshes.end(); it++) {
		auto& [trans, _, particles, __] = *it;

		if (cam->sphereInFrustum(applyTransform(trans), particles->radius)) {
			tempInstanced.push_back(*it);
		}
	}
	instancedMeshes = tempInstanced;
}

static void drawMesh(renderFlags& flags,
                     renderFramebuffer::ptr fb,
                     Program::ptr program,
                     glm::mat4& transform,
                     bool inverted,
                     gameMesh::ptr mesh)
{
	if (fb != nullptr && flags.stencil) {
		if (fb->drawn_meshes.size() < 0xff) {
			fb->drawn_meshes.push_back(mesh);
			glStencilFunc(GL_ALWAYS, fb->drawn_meshes.size(), ~0);

		} else {
			glStencilFunc(GL_ALWAYS, 0, ~0);
		}
	}

	glm::mat3 m_3x3_inv_transp =
		glm::transpose(glm::inverse(model_to_world(transform)));

	program->set("m", transform);
	program->set("m_3x3_inv_transp", m_3x3_inv_transp);

	set_material(program, mesh->comped_mesh);

	// enable()/disable() cache state, no need to worry about toggling
	// too much state if queue is sorted
	if (mesh->comped_mesh->factors.blend != material::blend_mode::Opaque) {
		// TODO: handle mask blends
		enable(GL_BLEND);
	} else {
		disable(GL_BLEND);
	}

	// TODO: need to keep track of the model face order
	if (flags.cull_faces) {
		setFaceOrder(inverted? GL_CW : GL_CCW);
	}

	bindVao(mesh->comped_mesh->vao);
	/*
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(2.0);
	enable(GL_LINE_SMOOTH);
	*/
	glDrawElements(GL_TRIANGLES,
	               mesh->comped_mesh->elements->size / sizeof(GLuint),
	               GL_UNSIGNED_INT,
	               (void*)mesh->comped_mesh->elements->offset);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static void drawMeshInstanced(renderFlags& flags,
                              renderFramebuffer::ptr fb,
                              Program::ptr program,
                              glm::mat4& transform,
                              bool inverted,
                              gameParticles::ptr particles,
                              gameMesh::ptr mesh)
{
	if (fb != nullptr && flags.stencil) {
		if (fb->drawn_meshes.size() < 0xff) {
			fb->drawn_meshes.push_back(mesh);
			glStencilFunc(GL_ALWAYS, fb->drawn_meshes.size(), ~0);

		} else {
			glStencilFunc(GL_ALWAYS, 0, ~0);
		}
	}

	glm::mat3 m_3x3_inv_transp =
		glm::transpose(glm::inverse(model_to_world(transform)));

	program->set("m", transform);
	program->set("m_3x3_inv_transp", m_3x3_inv_transp);

	set_material(program, mesh->comped_mesh);

	// enable()/disable() cache state, no need to worry about toggling
	// too much state if queue is sorted
	if (mesh->comped_mesh->factors.blend != material::blend_mode::Opaque) {
		// TODO: handle mask blends
		enable(GL_BLEND);
	} else {
		disable(GL_BLEND);
	}

	// TODO: need to keep track of the model face order
	if (flags.cull_faces) {
		setFaceOrder(inverted? GL_CW : GL_CCW);
	}

	bindVao(mesh->comped_mesh->vao);

#if GLSL_VERSION >= 140
	particles->syncBuffer();
	program->setUniformBlock("instanceTransforms", particles->ubuffer,
	                         UBO_INSTANCE_TRANSFORMS);
	glDrawElementsInstanced(
		GL_TRIANGLES,
		mesh->comped_mesh->elements->size / sizeof(GLuint),
		GL_UNSIGNED_INT,
		(void*)mesh->comped_mesh->elements->offset,
		particles->activeInstances);

#else
	for (unsigned i = 0; i < particles->activeInstances; i++) {
		// TODO: fallback
	}
#endif
}

unsigned renderQueue::flush(unsigned width,
                            unsigned height,
                            renderContext::ptr rctx,
                            renderFlags& flags)
{
	unsigned drawnMeshes = 0;

	if (flags.sort)       { sort(); }
	if (flags.cull_faces) { enable(GL_CULL_FACE); }

	cam->setViewport(width, height);
	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform();
	glm::mat4 v_inv = glm::inverse(view);

	flags.skinnedShader->bind();
	flags.skinnedShader->set("v", view);
	flags.skinnedShader->set("p", projection);
	flags.skinnedShader->set("v_inv", v_inv);
	flags.skinnedShader->set("cameraPosition", cam->position());

	for (auto& [skin, drawinfo] : skinnedMeshes) {
		for (auto& [transform, inverted, mesh] : drawinfo) {
			for (unsigned i = 0; i < skin->inverseBind.size(); i++) {
				float tim = SDL_GetTicks()/1000.f;
				gameObject::ptr temp = skin->joints[i];
				glm::mat4 accum = glm::mat4(1);

				for (; temp != skin && temp; temp = temp->parent.lock()) {
					accum = temp->getTransform(tim)*accum;
				}
				auto mat = accum*skin->inverseBind[i];

				std::string sloc = "joints["+std::to_string(i)+"]";
				if (!flags.skinnedShader->set(sloc, mat)) {
					std::cerr <<
						"NOTE: couldn't set joint matrix " << i
						<< ", too many joints/wrong shader?" << std::endl;
					break;
				}
			}

			set_irradiance_probe(
				nearest_irradiance_probe(applyTransform(transform)),
				flags.skinnedShader,
				rctx->atlases);
			drawMesh(flags, nullptr, flags.skinnedShader,
			         transform, inverted, mesh);
		}

		// TODO: check whether this skin is already synced
		DO_ERROR_CHECK();
	}

	flags.mainShader->bind();
	flags.mainShader->set("v", view);
	flags.mainShader->set("p", projection);
	flags.mainShader->set("v_inv", v_inv);
	flags.mainShader->set("cameraPosition", cam->position());

	for (auto& [transform, inverted, mesh] : meshes) {
		set_irradiance_probe(
			nearest_irradiance_probe(applyTransform(transform)),
			flags.mainShader, rctx->atlases);

		drawMesh(flags, nullptr, flags.mainShader, transform, inverted, mesh);
		drawnMeshes++;
	}

	meshes.clear();
	lights.clear();
	probes.clear();
	skinnedMeshes.clear();
	instancedMeshes.clear();

	return drawnMeshes;
}

unsigned renderQueue::flush(renderFramebuffer::ptr fb,
                            renderContext::ptr rctx,
                            renderFlags& flags)
{
	DO_ERROR_CHECK();
	unsigned drawnMeshes = 0;

	if (flags.sort) { sort(); }

	if (flags.cull_faces) {
		enable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
	}

	if (flags.stencil) {
		enable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	if (flags.depth) {
		enable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

	} else {
		disable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
	}

	DO_ERROR_CHECK();
	assert(fb != nullptr);
	fb->framebuffer->bind();

	disable(GL_SCISSOR_TEST);
	glViewport(0, 0, fb->width, fb->height);
	glScissor(0, 0, fb->width, fb->height);

	cam->setViewport(fb->width, fb->height);
	glm::mat4 view = cam->viewTransform();
	glm::mat4 projection = cam->projectionTransform();
	glm::mat4 v_inv = glm::inverse(view);

	flags.skinnedShader->bind();
	flags.skinnedShader->set("v", view);
	flags.skinnedShader->set("p", projection);
	flags.skinnedShader->set("v_inv", v_inv);
	flags.skinnedShader->set("cameraPosition", cam->position());
	shaderSync(flags.skinnedShader, rctx);
	gameSkin::ptr lastskin = nullptr;

	for (auto& [skin, drawinfo] : skinnedMeshes) {
		float offset = -1.0;

		for (auto& [transform, inverted, mesh] : drawinfo) {
			// TODO: have time offset field for animated meshes
			if (offset != 0.0) {
				for (unsigned i = 0; i < skin->inverseBind.size(); i++) {
					float tim = SDL_GetTicks()/1000.f;
					gameObject::ptr temp = skin->joints[i];
					glm::mat4 accum = glm::mat4(1);

					for (; temp != skin && temp; temp = temp->parent.lock()) {
						accum = temp->getTransform(tim)*accum;
					}

					auto mat = accum*skin->inverseBind[i];
					std::string sloc = "joints["+std::to_string(i)+"]";

					if (!flags.skinnedShader->set(sloc, mat)) {
						std::cerr <<
							"NOTE: couldn't set joint matrix " << i
							<< ", too many joints/wrong shader?" << std::endl;
						break;
					}
				}

				// TODO: s/0.0/current offset
				offset = 0.0;
			}

			set_irradiance_probe(
				nearest_irradiance_probe(applyTransform(transform)),
				flags.skinnedShader,
				rctx->atlases);
			drawMesh(flags, fb, flags.skinnedShader, transform, inverted, mesh);
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
	shaderSync(flags.mainShader, rctx);

	for (auto& [transform, inverted, mesh] : meshes) {
		set_irradiance_probe(
			nearest_irradiance_probe(applyTransform(transform)),
			flags.mainShader, rctx->atlases);
		drawMesh(flags, fb, flags.mainShader, transform, inverted, mesh);
		drawnMeshes++;
	}

	flags.instancedShader->bind();
	flags.instancedShader->set("v", view);
	flags.instancedShader->set("p", projection);
	flags.instancedShader->set("v_inv", v_inv);
	flags.instancedShader->set("cameraPosition", cam->position());
	shaderSync(flags.instancedShader, rctx);

	for (auto& [transform, inverted, particleSystem, mesh] : instancedMeshes) {
		set_irradiance_probe(
			nearest_irradiance_probe(applyTransform(transform)),
			flags.mainShader, rctx->atlases);
		drawMeshInstanced(flags, fb, flags.instancedShader,
		                  transform, inverted, particleSystem, mesh);
		drawnMeshes += particleSystem->activeInstances;
	}

	// TODO: clear()
	meshes.clear();
	lights.clear();
	probes.clear();
	skinnedMeshes.clear();
	instancedMeshes.clear();

	return drawnMeshes;
}

#if GLSL_VERSION < 140 /* < opengl 3.1, use plain uniform indexes */
static void syncPlainUniforms(Program::ptr program,
	                          renderContext::ptr rctx,
	                          std::vector<gameLightPoint::ptr>& points,
	                          std::vector<gameLightSpot::ptr>& spots,
	                          std::vector<gameLightDirectional::ptr>& directionals)
{
	size_t pactive = min(MAX_LIGHTS, points.size());
	size_t sactive = min(MAX_LIGHTS, spots.size());
	size_t dactive = min(MAX_LIGHTS, directionals.size());

	program->set("active_points",       (GLint)pactive);
	program->set("active_spots",        (GLint)sactive);
	program->set("active_directionals", (GLint)dactive);

	for (size_t i = 0; i < pactive; i++) {
		gameLightPoint::ptr light = points[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "points[" + std::to_string(i) + "]";

		program->set(locstr + ".position",      light->transform.position);
		program->set(locstr + ".diffuse",       light->diffuse);
		program->set(locstr + ".intensity",     light->intensity);
		program->set(locstr + ".radius",        light->radius);
		program->set(locstr + ".casts_shadows", light->casts_shadows);

		if (light->casts_shadows) {
			for (unsigned k = 0; k < 6; k++) {
				std::string sloc = locstr + ".shadowmap[" + std::to_string(k) + "]";
				auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap[k]);
				program->set(sloc, vec);
			}
		}
	}

	for (size_t i = 0; i < sactive; i++) {
		gameLightSpot::ptr light = spots[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "spots[" + std::to_string(i) + "]";
		// light points toward +X by default
		glm::vec3 rotvec =
			glm::mat3_cast(light->transform.rotation) * glm::vec3(1, 0, 0);

		program->set(locstr + ".position",      light->transform.position);
		program->set(locstr + ".diffuse",       light->diffuse);
		program->set(locstr + ".direction",     rotvec);
		program->set(locstr + ".intensity",     light->intensity);
		program->set(locstr + ".radius",        light->radius);
		program->set(locstr + ".angle",         light->angle);
		program->set(locstr + ".casts_shadows", light->casts_shadows);

		if (light->casts_shadows) {
			std::string sloc = locstr + ".shadowmap";
			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
			program->set(sloc, vec);
		}
	}

	for (size_t i = 0; i < dactive; i++) {
		gameLightDirectional::ptr light = directionals[i];

		// TODO: also updating all these uniforms can't be very efficient for a lot of
		//       moving point lights... maybe look into how uniform buffer objects work
		std::string locstr = "directionals[" + std::to_string(i) + "]";
		// light points toward +X by default
		glm::vec3 rotvec =
			glm::mat3_cast(light->transform.rotation) * glm::vec3(1, 0, 0);

		// position is ignored in the shader, but might as well set it anyway
		// TODO: probably remove position member from directional lights
		program->set(locstr + ".position",      light->transform.position);
		program->set(locstr + ".diffuse",       light->diffuse);
		program->set(locstr + ".direction",     rotvec);
		program->set(locstr + ".intensity",     light->intensity);
		program->set(locstr + ".casts_shadows", light->casts_shadows);

		if (light->casts_shadows) {
			std::string sloc = locstr + ".shadowmap";
			auto vec = rctx->atlases.shadows->tex_vector(light->shadowmap);
			program->set(sloc, vec);
		}
	}
}
#endif

typedef std::vector<std::pair<glm::mat4&, gameLightPoint::ptr>>       pointList;
typedef std::vector<std::pair<glm::mat4&, gameLightSpot::ptr>>        spotList;
typedef std::vector<std::pair<glm::mat4&, gameLightDirectional::ptr>> dirList;
typedef std::tuple<pointList, spotList, dirList> lightLists;

static inline float rectScale(float R, float d) {
	return 1.f / cos(d/R);
}

// TODO: possibly move this to it's own file
#include <grend/plane.hpp>
void grendx::buildTilemap(renderQueue& queue, renderContext::ptr rctx) {
	lights_std140&      lightbuf = rctx->lightBufferCtx;
	light_tiles_std140& pointbuf = rctx->pointTilesCtx;
	light_tiles_std140& spotbuf  = rctx->spotTilesCtx;

	camera::ptr cam = queue.cam;
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
	auto buildTiles = [&] (gameLight::ptr lit, glm::mat4& trans, unsigned idx) {
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

				if (lit->lightType == gameLight::lightTypes::Point) {
					unsigned cur  = pointbuf.indexes[clusidx];
					unsigned next = cur + 1;

					if (next < MAX_LIGHTS) {
						pointbuf.indexes[clusidx]        = next;
						pointbuf.indexes[clusidx + next] = idx;
					}

				} else if (lit->lightType == gameLight::lightTypes::Spot) {
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

	for (auto& [trans, _, lit] : queue.lights) {
		glm::vec3 pos = applyTransform(trans);

		if (activePoints < MAX_POINT_LIGHT_OBJECTS &&
		    lit->lightType == gameLight::lightTypes::Point)
		{
			gameLightPoint::ptr plit =
				std::dynamic_pointer_cast<gameLightPoint>(lit);

			packLight(plit,
					  lightbuf.upoint_lights + activePoints,
					  rctx, trans);
			buildTiles(lit, trans, activePoints);
			activePoints++;

		} else if (activeSpots < MAX_SPOT_LIGHT_OBJECTS
		           && lit->lightType == gameLight::lightTypes::Spot)
		{
			gameLightSpot::ptr slit =
				std::dynamic_pointer_cast<gameLightSpot>(lit);

			packLight(slit,
					  lightbuf.uspot_lights + activeSpots,
					  rctx, trans);
			buildTiles(lit, trans, activeSpots);
			activeSpots++;

		} else if (activeDirs < MAX_DIRECTIONAL_LIGHT_OBJECTS
		          && lit->lightType == gameLight::lightTypes::Directional)
		{
			gameLightDirectional::ptr dlit =
				std::dynamic_pointer_cast<gameLightDirectional>(lit);

			packLight(dlit,
					  lightbuf.udirectional_lights + activeDirs,
					  rctx, trans);
			activeDirs++;
		}
	}

	lightbuf.uactive_point_lights       = activePoints;
	lightbuf.uactive_spot_lights        = activeSpots;
	lightbuf.uactive_directional_lights = activeDirs;

	rctx->lightBuffer->update(&lightbuf, 0, sizeof(lightbuf));
	rctx->pointTiles->update(&pointbuf,  0, sizeof(pointbuf));
	rctx->spotTiles->update(&spotbuf,    0, sizeof(spotbuf));
}

void renderQueue::updateReflectionProbe(renderContext::ptr rctx) {
	auto refprobe = nearest_reflection_probe(cam->position());

	// TODO: set the real transform
	glm::mat4 xxx(1);
	packRefprobe(refprobe, &rctx->lightBufferCtx, rctx, xxx);
	//rctx->lightBuffer->update(&rctx->lightBufferCtx, 16, 528);
	rctx->lightBuffer->update(&rctx->lightBufferCtx, 0, sizeof(rctx->lightBufferCtx));
}

void renderQueue::shaderSync(Program::ptr program, renderContext::ptr rctx) {

#if GLSL_VERSION >= 140
	program->setUniformBlock("lights",      rctx->lightBuffer, UBO_LIGHT_INFO);
	program->setUniformBlock("point_light_tiles", rctx->pointTiles,
	                         UBO_POINT_LIGHT_TILES);
	program->setUniformBlock("spot_light_tiles", rctx->spotTiles,
	                         UBO_SPOT_LIGHT_TILES);

	DO_ERROR_CHECK();

#else
	pointList point_lights;
	spotList  spot_lights;
	dirList   directional_lights;

	for (auto& [trans, _, light] : lights) {
		switch (light->lightType) {
			case gameLight::lightTypes::Point:
				{
					gameLightPoint::ptr plit =
						std::dynamic_pointer_cast<gameLightPoint>(light);
					point_lights.push_back({trans, plit});
				}
				break;

			case gameLight::lightTypes::Spot:
				{
					gameLightSpot::ptr slit =
						std::dynamic_pointer_cast<gameLightSpot>(light);
					spot_lights.push_back({trans, slit});
				}
				break;

			case gameLight::lightTypes::Directional:
				{
					gameLightDirectional::ptr dlit =
						std::dynamic_pointer_cast<gameLightDirectional>(light);
					directional_lights.push_back({trans, dlit});
				}
				break;

			default:
				break;
		}
	}

	syncPlainUniforms(program, rctx, point_lights,
	                  spot_lights, directional_lights);

	set_reflection_probe(refprobe, program, rctx->atlases);
	DO_ERROR_CHECK();
#endif

	program->set("renderWidth",  (float)rctx->framebuffer->width);
	program->set("renderHeight", (float)rctx->framebuffer->height);
	program->set("lightThreshold", rctx->lightThreshold);

	glActiveTexture(GL_TEXTURE6);
	rctx->atlases.reflections->color_tex->bind();
	program->set("reflection_atlas", 6);

	glActiveTexture(GL_TEXTURE7);
	rctx->atlases.shadows->depth_tex->bind();
	program->set("shadowmap_atlas", 7);
	DO_ERROR_CHECK();

	glActiveTexture(GL_TEXTURE8);
	rctx->atlases.irradiance->color_tex->bind();
	program->set("irradiance_atlas", 8);
	DO_ERROR_CHECK();
}

gameReflectionProbe::ptr renderQueue::nearest_reflection_probe(glm::vec3 pos) {
	gameReflectionProbe::ptr ret = nullptr;
	float mindist = HUGE_VALF;

	// TODO: optimize this, O(N) is meh
	for (auto& [_, __, p] : probes) {
		float dist = glm::distance(pos, p->transform.position);
		if (!ret || dist < mindist) {
			mindist = dist;
			ret = p;
		}
	}

	return ret;
}

gameIrradianceProbe::ptr renderQueue::nearest_irradiance_probe(glm::vec3 pos) {
	gameIrradianceProbe::ptr ret = nullptr;
	float mindist = HUGE_VALF;

	// TODO: optimize this, O(N) is meh
	for (auto& [_, __, p] : irradProbes) {
		float dist = glm::distance(pos, p->transform.position);
		if (!ret || dist < mindist) {
			mindist = dist;
			ret = p;
		}
	}

	return ret;
}

void renderQueue::set_reflection_probe(gameReflectionProbe::ptr probe,
                                       Program::ptr program,
                                       renderAtlases& atlases)
{
	if (!probe) {
		return;
	}

	for (unsigned k = 0; k < 5; k++) {
		for (unsigned i = 0; i < 6; i++) {
			std::string sloc =
				"reflection_probe["+std::to_string(k*6 + i)+"]";
			glm::vec3 facevec;

			if (k == 0) {
				facevec = atlases.reflections->tex_vector(probe->faces[k][i]);
			} else {
				facevec = atlases.irradiance->tex_vector(probe->faces[k][i]);
			}

			program->set(sloc, facevec);
			DO_ERROR_CHECK();
		}
	}

	program->set("refboxMin",        probe->transform.position + probe->boundingBox.min);
	program->set("refboxMax",        probe->transform.position + probe->boundingBox.max);
	program->set("refprobePosition", probe->transform.position);
}

void renderQueue::set_irradiance_probe(gameIrradianceProbe::ptr probe,
                                       Program::ptr program,
                                       renderAtlases& atlases)
{
	if (!probe) {
		return;
	}

	for (unsigned i = 0; i < 6; i++) {
		std::string sloc = "irradiance_probe[" + std::to_string(i) + "]";
		glm::vec3 facevec = atlases.irradiance->tex_vector(probe->faces[i]); 
		program->set(sloc, facevec);
		DO_ERROR_CHECK();
	}

	program->set("radboxMin",        probe->transform.position + probe->boundingBox.min);
	program->set("radboxMax",        probe->transform.position + probe->boundingBox.max);
	program->set("radprobePosition", probe->transform.position);
}
