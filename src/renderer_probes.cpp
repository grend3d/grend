#include <grend/engine.hpp>
#include <grend/gameModel.hpp>
#include <grend/utility.hpp>

using namespace grendx;

static const glm::vec3 cube_dirs[] = {
	// negative X, Y, Z
	{-1,  0,  0},
	{ 0, -1,  0},
	{ 0,  0, -1},

	// positive X, Y, Z
	{ 1,  0,  0},
	{ 0,  1,  0},
	{ 0,  0,  1},
};

static const glm::vec3 cube_up[] = {
	// negative X, Y, Z
	{ 0,  1,  0},
	{ 0,  0,  1},
	{ 0,  1,  0},

	// positive X, Y, Z
	{ 0,  1,  0},
	{ 0,  0,  1},
	{ 0,  1,  0},
};


void renderer::drawSkybox(void) {
#if 0
	rend.set_shader(rend.shaders["skybox"]);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

#ifdef ENABLE_FACE_CULLING
	// TODO: toggle per-model
	rend.glman.disable(GL_CULL_FACE);
#endif

	glActiveTexture(GL_TEXTURE4);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.first);
	skybox->bind(GL_TEXTURE_CUBE_MAP);
	rend.shader->set("skytexture", 4);
	//shader_obj.set("skytexture", 4);
	DO_ERROR_CHECK();

	rend.set_mvp(glm::mat4(0), glm::mat4(glm::mat3(view)), projection);
	//draw_model("unit_cube", glm::mat4(1));
	//draw_mesh("unit_cube.default", glm::mat4(0));
	struct draw_attributes attrs = {
		.name = "unit_cube",
		.transform = glm::mat4(0),
	};
	rend.draw_mesh("unit_cube.default", &attrs);
	glDepthMask(GL_TRUE);
#endif
}

void renderer::drawRefprobeSkybox(glm::mat4 view, glm::mat4 proj) {
#if 0
	rend.set_shader(rend.shaders["skybox"]);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

#ifdef ENABLE_FACE_CULLING
	// TODO: toggle per-model
	rend.glman.disable(GL_CULL_FACE);
#endif

	glActiveTexture(GL_TEXTURE4);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.first);
	skybox->bind(GL_TEXTURE_CUBE_MAP);
	rend.shader->set("skytexture", 4);
	//shader_obj.set("skytexture", 4);
	DO_ERROR_CHECK();

	rend.set_mvp(glm::mat4(0), glm::mat4(glm::mat3(view)), proj);
	//draw_model("unit_cube", glm::mat4(1));
	//draw_mesh("unit_cube.default", glm::mat4(0));
	struct draw_attributes attrs = {
		.name = "unit_cube",
		.transform = glm::mat4(0),
	};
	rend.draw_mesh("unit_cube.default", &attrs);
	glDepthMask(GL_TRUE);
#endif
}

void renderer::drawShadowCubeMap(gameLightPoint::ptr light) {
// TODO: do this, rn focusing on getting a basic scene drawn
#if 0
	rend.glman.enable(GL_SCISSOR_TEST);
	rend.glman.enable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	rend.set_shader(rend.shaders["shadow"]);

	if (!plit.casts_shadows || (plit.static_shadows && plit.shadows_rendered))
		continue;

	for (unsigned i = 0; i < 6; i++) {
		if (!rend.shadow_atlas->bind_atlas_fb(plit.shadowmap[i])) {
			std::cerr
				<< "render_light_maps(): couldn't bind shadowmap FB"
				<< std::endl;
			continue;
		}

		view = glm::lookAt(plit.position,
				plit.position + cube_dirs[i],
				cube_up[i]);
		rend.set_mvp(glm::mat4(1), view, projection);
		glClear(GL_DEPTH_BUFFER_BIT);

		render_static(ctx);
		render_players(ctx);
		render_dynamic(ctx);
		//editor.render_map_models(&rend, ctx);
		rend.dqueue_sort_draws(plit.position);
		rend.dqueue_flush_draws();
		DO_ERROR_CHECK();
	}

	plit.shadows_rendered = true;
#endif
}

void renderer::drawShadowMap(gameLightSpot::ptr light) {
// TODO:
#if 0
	rend.glman.enable(GL_SCISSOR_TEST);
	rend.glman.enable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	if (!slit.casts_shadows || (slit.static_shadows && slit.shadows_rendered))
		continue;

	if (!rend.shadow_atlas->bind_atlas_fb(slit.shadowmap)) {
		std::cerr
			<< "render_light_maps(): couldn't bind shadowmap FB"
			<< std::endl;
		continue;
	}

	glClear(GL_DEPTH_BUFFER_BIT);

	//glm::vec3 right = glm::vec3(slit.direction.y, slit.direction.z, slit.direction.x);
	//glm::vec3 up = glm::normalize(glm::cross(slit.direction, right));
	glm::vec3 up = glm::vec3(0, 0, 1);
	view = glm::lookAt(slit.position,
			slit.position - slit.direction,
			up);

	// TODO: adjust perspective to spot light angle
	// float angle = acos(slit.angle);
	projection = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.f);
	rend.set_mvp(glm::mat4(1), view, projection);

	render_static(ctx);
	render_players(ctx);
	render_dynamic(ctx);
	rend.dqueue_sort_draws(slit.position);
	rend.dqueue_flush_draws();
	DO_ERROR_CHECK();

	slit.shadows_rendered = true;
#endif
}

void renderer::drawShadowMap(gameLightDirectional::ptr light) {
	//TODO:
}

void renderer::drawReflectionProbe(gameReflectionProbe::ptr probe) {
// TODO:
#if 0
	rend.glman.enable(GL_SCISSOR_TEST);
	rend.glman.enable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	rend.set_shader(rend.shaders["refprobe"]);
	rend.update_lights();

	glActiveTexture(GL_TEXTURE7);
	rend.shadow_atlas->depth_tex->bind();
	rend.shader->set("shadowmap_atlas", 7);

	rend.shader->set("skytexture", 4);
	rend.shader->set("time_ms", SDL_GetTicks() * 1.f);

	for (auto& [id, probe] : rend.ref_probes) {
		if (probe.is_static && probe.have_map)
			continue;

		for (unsigned i = 0; i < 6; i++) {
			if (!rend.reflection_atlas->bind_atlas_fb(probe.faces[i])) {
				std::cerr
					<< "render_light_maps(): couldn't bind reflection FB"
					<< std::endl;
				continue;
			}

			rend.set_shader(rend.shaders["refprobe"]);
			view = glm::lookAt(probe.position,
					probe.position + cube_dirs[i],
					cube_up[i]);
			rend.set_mvp(glm::mat4(1), view, projection);

			/*
			   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			   SDL_Die("incomplete!");
			   }
			   */

			glm::vec3 bugc = (cube_dirs[i] + glm::vec3(1)) / glm::vec3(2);

			glClearColor(bugc.x, bugc.y, bugc.z, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			DO_ERROR_CHECK();

			render_static(ctx);
			render_players(ctx);
			render_dynamic(ctx);
			//editor.render_map_models(&rend, ctx);
			rend.dqueue_sort_draws(probe.position);
			rend.dqueue_flush_draws();
			DO_ERROR_CHECK();
			render_refprobe_skybox(ctx, view, projection);

			probe.have_map = true;
		}
	}
#endif
}
