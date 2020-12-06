#pragma once

#include <grend/renderFramebuffer.hpp>
#include <grend/renderPostStage.hpp>
#include <grend/gameObject.hpp>
#include <grend/glManager.hpp>
#include <grend/sdlContext.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/gameModel.hpp>
#include <grend/textureAtlas.hpp>
#include <grend/quadtree.hpp>
#include <grend/camera.hpp>

#include <list>
#include <memory>
#include <tuple>

#include <stdint.h>

namespace grendx {

#ifndef MAX_LIGHTS
#define MAX_LIGHTS 8
#endif

// TODO: need some unified location to put this, there's a duplicated definition
//       of this in shaders/lib/shading-uniforms.glsl
#if defined(CLUSTERED_LIGHTS) && (GLSL_VERSION == 300 || GLSL_VERSION >= 430)
// for clustered, tiled, number of possible light objects available (ie. in view)
#ifndef MAX_POINT_LIGHT_OBJECTS
#define MAX_POINT_LIGHT_OBJECTS 1024
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS
#define MAX_SPOT_LIGHT_OBJECTS 1024
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS
#define MAX_DIRECTIONAL_LIGHT_OBJECTS 32
#endif

#else // if < opengl 4.3
#ifndef MAX_POINT_LIGHT_OBJECTS
#define MAX_POINT_LIGHT_OBJECTS 64
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS
#define MAX_SPOT_LIGHT_OBJECTS 32
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS
#define MAX_DIRECTIONAL_LIGHT_OBJECTS 4
#endif
#endif // < opengl 4.3

struct point_std140 {
	GLfloat position[4];   // 0
	GLfloat diffuse[4];    // 16
	GLfloat intensity;     // 32
	GLfloat radius;        // 36
	GLuint  casts_shadows; // 40
	GLfloat padding;       // 44, pad to 48
	GLfloat shadowmap[24]; // 48, end 144
} __attribute__((packed));

struct spot_std140 {
	GLfloat position[4];   // 0
	GLfloat diffuse[4];    // 16
	GLfloat direction[3];  // 32
	GLfloat intensity;     // 44
	GLfloat radius;        // 48
	GLfloat angle;         // 52
	GLuint  casts_shadows; // 56
	GLuint  padding;       // 60, pad to 64
	GLfloat shadowmap[4];  // 64, end 80
} __attribute__((packed));

struct directional_std140 {
	GLfloat position[4];   // 0
	GLfloat diffuse[4];    // 16
	GLfloat direction[3];  // 32
	GLfloat intensity;     // 44
	GLuint  casts_shadows; // 48
	GLfloat padding[3];    // 52, pad to 64
	GLfloat shadowmap[4];  // 64, end 80
} __attribute__((packed));

struct lights_std140 {
	GLuint uactive_point_lights;       // 0
	GLuint uactive_spot_lights;        // 4
	GLuint uactive_directional_lights; // 8
	GLuint padding;                    // 12, end 16

	GLfloat reflection_probe[30 * 4];
	GLfloat refboxMin[4];
	GLfloat refboxMax[4];
	GLfloat refprobePosition[4];       // end 528 + 16

	point_std140 upoint_lights[MAX_POINT_LIGHT_OBJECTS];
	spot_std140 uspot_lights[MAX_SPOT_LIGHT_OBJECTS];
	directional_std140 udirectional_lights[MAX_DIRECTIONAL_LIGHT_OBJECTS];
} __attribute__((packed));

struct light_tiles_std140 {
	GLfloat point_tiles[9*16*MAX_LIGHTS];
	GLfloat spot_tiles[9*16*MAX_LIGHTS];
} __attribute__((packed));

class skybox {
	public:
		skybox(); 

		void draw(camera::ptr cam, unsigned width, unsigned height);
		void draw(camera::ptr cam, renderFramebuffer::ptr fb);

		gameModel::ptr model;
		Program::ptr program;
		Texture::ptr map;
};

class renderAtlases {
	public:
		renderAtlases(unsigned refSize = 2048,
		              unsigned shadowSize = 2048,
		              unsigned irradSize = 1024,
		              unsigned coeffSize = 1024)
		{
			reflections = atlas::ptr(new atlas(refSize));
			shadows     = atlas::ptr(new atlas(shadowSize, atlas::mode::Depth));
			irradiance  = atlas::ptr(new atlas(irradSize));
			irradianceCoefficients = atlas::ptr(new atlas(coeffSize));
		}

		atlas::ptr reflections;
		atlas::ptr shadows;
		atlas::ptr irradiance;
		atlas::ptr irradianceCoefficients;
};

struct renderFlags {
	Program::ptr mainShader;
	Program::ptr skinnedShader;

	bool cull_faces = true;
	bool sort       = true;
	bool stencil    = true;
	bool depth      = true;
	bool syncshader = true;
};

class renderContext {
	struct renderFlags flags;

	public:
		typedef std::shared_ptr<renderContext> ptr;
		typedef std::weak_ptr<renderContext> weakptr;

		renderContext(context& ctx);
		~renderContext() { };
		void loadShaders(void);

		// TODO: 
		// see drawn_entities below
		void newframe(void);
		// look up stencil buffer index in drawn objects
		gameMesh::ptr index(unsigned idx);

		// XXX
		void setFlags(const renderFlags& newflags);
		struct renderFlags getDefaultFlags(std::string name="main");
		const renderFlags& getFlags(void);

		// TODO: swap between these
		renderFramebuffer::ptr framebuffer;
		//renderFramebuffer last_frame;

		// (actual) screen size
		int screen_x, screen_y;

		// sky box
		skybox defaultSkybox;
		renderAtlases atlases;

		// XXX: loaded shaders, here so they can be accessed from the editor
		std::map<std::string, Program::ptr> shaders;
		Buffer::ptr lightBuffer;
		Buffer::ptr lightTiles;

		float lightThreshold = 0.05;
		lights_std140      lightBufferCtx;
		light_tiles_std140 lightTilesCtx;

	protected:
		Program::ptr shader;

		/*
		gameReflectionProbe::ptr get_nearest_refprobes(glm::vec3 pos);
		// list of models to draw
		// TODO: meshes, need to pair materials with meshes though, which
		//       currently is part of the model classes
		//       possible solution is to have a global namespace for materials,
		//       which would be easier to work with probably
		//       (also might make unintentional dependencies easier though...)
		//std::vector<gameMesh::ptr> draw_queue;
		// sorted after dqueue_sort_draws(), and flushed along with
		// dqueue_flush_draws(), this is sorted back-to-front rather than
		// front-to-back
		std::vector<gameMesh::ptr> transparent_draws;
		std::vector<gameLight::ptr> active_lights;
		std::vector<gameReflectionProbe::ptr> active_refprobes;
		*/
};

class renderQueue {
	public:
		renderQueue(camera::ptr cam) { setCamera(cam); };
		renderQueue(renderQueue& other) {
			meshes        = other.meshes;
			skinnedMeshes = other.skinnedMeshes;
			lights        = other.lights;
			probes        = other.probes;
			cam           = other.cam;
		}

		void add(gameObject::ptr obj,
		         float animTime = 0.0,
		         glm::mat4 trans = glm::mat4(1),
		         bool inverted = false);
		void addSkinned(gameObject::ptr obj,
		                gameSkin::ptr skin,
		                float animTime = 0.0,
		                glm::mat4 trans = glm::mat4(1),
		                bool inverted = false);
		void updateLights(Program::ptr program, renderContext::ptr rctx);
		void updateReflections(Program::ptr program, renderContext::ptr rctx);
		void updateReflectionProbe(renderContext::ptr rctx);
		void sort(void);
		void cull(unsigned width, unsigned height, float lightext);
		unsigned flush(renderFramebuffer::ptr fb, renderContext::ptr rctx);
		unsigned flush(unsigned width, unsigned height, renderContext::ptr rctx);
		void shaderSync(Program::ptr program, renderContext::ptr rctx);
		void setCamera(camera::ptr newcam) { cam = newcam; };

		camera::ptr cam = nullptr;

		// mat4 is calculated transform for the position of the node in the tree
		// bool is inverted flag
		std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>> meshes;
		std::map<gameSkin::ptr, std::vector<std::tuple<glm::mat4, bool, gameMesh::ptr>>> skinnedMeshes;
		std::vector<std::tuple<glm::mat4, bool, gameLight::ptr>> lights;
		std::vector<std::tuple<glm::mat4, bool, gameReflectionProbe::ptr>> probes;
		std::vector<std::tuple<glm::mat4, bool, gameIrradianceProbe::ptr>> irradProbes;

	private:
		gameReflectionProbe::ptr nearest_reflection_probe(glm::vec3 pos);
		gameIrradianceProbe::ptr nearest_irradiance_probe(glm::vec3 pos);
		void set_reflection_probe(gameReflectionProbe::ptr probe,
		                          Program::ptr program,
		                          renderAtlases& atlases);
		void set_irradiance_probe(gameIrradianceProbe::ptr probe,
		                          Program::ptr program,
		                          renderAtlases& atlases);
};

void drawShadowCubeMap(renderQueue& queue,
                       gameLightPoint::ptr light,
					   renderContext::ptr rctx);
void drawReflectionProbe(renderQueue& queue,
                         gameReflectionProbe::ptr probe,
                         renderContext::ptr rctx);
void drawIrradianceProbe(renderQueue& queue,
                         gameIrradianceProbe::ptr probe,
                         renderContext::ptr rctx);

void buildTilemap(renderQueue& queue, renderContext::ptr rctx);

void packLight(gameLightPoint::ptr light, point_std140 *p,
               renderContext::ptr rctx, glm::mat4& trans);
void packLight(gameLightSpot::ptr light, spot_std140 *p,
               renderContext::ptr rctx, glm::mat4& trans);
void packLight(gameLightDirectional::ptr light, directional_std140 *p,
               renderContext::ptr rctx, glm::mat4& trans);
void packRefprobe(gameReflectionProbe::ptr probe, lights_std140 *p,
                  renderContext::ptr rctx, glm::mat4& trans);

float light_extent(struct point_light *p, float threshold=0.03);
float light_extent(struct spot_light *s, float threshold=0.03);
glm::mat4 model_to_world(glm::mat4 model);

void set_material(Program::ptr program, compiledMesh::ptr mesh);
void set_default_material(Program::ptr program);
void invalidateLightMaps(gameObject::ptr tree);

// namespace grendx
}
