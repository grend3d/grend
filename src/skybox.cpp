#include <grend-config.h>

#include <grend/engine.hpp>
#include <grend/glManager.hpp>
#include <grend/geometryGeneration.hpp> // TODO: camelCase

using namespace grendx;

skyRender::~skyRender() {};
skyRenderCube::~skyRenderCube() {};
skyRenderHDRI::~skyRenderHDRI() {};

// TODO: just look for recognized extensions
skyRenderCube::skyRenderCube(const std::string& cubepath,
                             const std::string& extension)
	: skyRender()
{
	model = generate_cuboid(1, 1, 1); // unit cube
	compileModel("defaultSkyboxCuboid", model);
	// TODO: need like a "bind new meshes" function
	bindModel(model);
	//bindCookedMeshes();

	Shader::parameters nullopts; // XXX
	program = loadProgram(
		GR_PREFIX "shaders/baked/skybox.vert",
		//GR_PREFIX "shaders/baked/dynamic-skybox.frag",
		GR_PREFIX "shaders/baked/skybox.frag",
		nullopts
	);

	program->attribute("v_position", VAO_VERTICES);
	if (!program->link()) {
		// TODO: maybe don't throw exceptions
		throw std::logic_error(program->log());
	}

	map = genTexture();
	map->cubemap(cubepath, extension);
}

void skyRenderCube::draw(camera::ptr cam, unsigned width, unsigned height) {
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	disable(GL_CULL_FACE);

	program->bind();
	program->set("m", glm::mat4(0));
	program->set("v", glm::mat4(glm::mat3(cam->viewTransform())));
	program->set("p", cam->projectionTransform());

	glActiveTexture(TEX_GL_SKYBOX);
	map->bind(GL_TEXTURE_CUBE_MAP);
	program->set("skytexture", TEXU_SKYBOX);

	auto node = model->getNode("mesh");
	sceneMesh::ptr mesh = ref_cast<sceneMesh>(node);
	auto& cmesh = mesh->comped_mesh;

	bindVao(cmesh->vao);
	glDrawElements(GL_TRIANGLES, cmesh->elements->currentSize/sizeof(uint32_t), GL_UNSIGNED_INT, 0);
	glDepthMask(GL_TRUE);
}

skyRenderHDRI::skyRenderHDRI(const std::string& hdrPath)
	: skyRender()
{
	model = generate_cuboid(1, 1, 1); // unit cube
	compileModel("defaultSkyboxCuboid", model);

	Shader::parameters nullopts; // XXX
	program = loadProgram(
		GR_PREFIX "shaders/baked/skybox.vert",
		GR_PREFIX "shaders/baked/skybox-eqrect.frag",
		nullopts
	);

	program->attribute("v_position", VAO_VERTICES);
	if (!program->link()) {
		// TODO: maybe don't throw exceptions
		throw std::logic_error(program->log());
	}

	textureData data(hdrPath);

	map = genTexture();
	map->buffer(data);
}

void skyRenderHDRI::draw(camera::ptr cam, unsigned width, unsigned height) {
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	disable(GL_CULL_FACE);

	program->bind();
	program->set("m", glm::mat4(0));
	program->set("v", glm::mat4(glm::mat3(cam->viewTransform())));
	program->set("p", cam->projectionTransform());

	glActiveTexture(TEX_GL_SKYBOX);
	map->bind();
	program->set("skytexture", TEXU_SKYBOX);

	auto node = model->getNode("mesh");
	sceneMesh::ptr mesh = ref_cast<sceneMesh>(node);
	auto& cmesh = mesh->comped_mesh;

	bindVao(cmesh->vao);
	glDrawElements(GL_TRIANGLES, cmesh->elements->currentSize/sizeof(uint32_t), GL_UNSIGNED_INT, 0);
	glDepthMask(GL_TRUE);
}
