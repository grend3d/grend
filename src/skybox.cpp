#include <grend/engine.hpp>
#include <grend/gl_manager.hpp>
#include <grend/geometry-gen.hpp> // TODO: camelCase

using namespace grendx;

skybox::skybox() {
	model = generate_cuboid(1, 1, 1); // unit cube
	compile_model("defaultSkyboxCuboid", model);
	// TODO: need like a "bind new meshes" function
	bind_cooked_meshes();

	program = load_program(
		GR_PREFIX "shaders/out/skybox.vert",
		GR_PREFIX "shaders/out/skybox.frag"
	);

	program->attribute("v_position", VAO_VERTICES);
	if (!program->link()) {
		// TODO: maybe don't throw exceptions
		throw std::logic_error(program->log());
	}

	/*
	glBindAttribLocation(program->obj, 0, "v_position");
	link_program(program);
	*/

	map = gen_texture();
	map->cubemap(GR_PREFIX "assets/tex/cubes/default/", ".png");
}

void skybox::draw(camera::ptr cam, unsigned width, unsigned height) {
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	disable(GL_CULL_FACE);

	program->bind();
	program->set("m", glm::mat4(0));
	program->set("v", glm::mat4(glm::mat3(cam->viewTransform())));
	program->set("p", cam->projectionTransform());

	glActiveTexture(GL_TEXTURE9);
	map->bind(GL_TEXTURE_CUBE_MAP);
	program->set("skytexture", 9);

	gameMesh::ptr mesh =
		std::dynamic_pointer_cast<gameMesh>(model->getNode("mesh"));
	auto& cmesh = mesh->comped_mesh;

	bind_vao(cmesh->vao);
	glDrawElements(GL_TRIANGLES, cmesh->elements_size,
			GL_UNSIGNED_INT, cmesh->elements_offset);
	glDepthMask(GL_TRUE);
}

void skybox::draw(camera::ptr cam, renderFramebuffer::ptr fb) {
	draw(cam, fb->width, fb->height);
}
