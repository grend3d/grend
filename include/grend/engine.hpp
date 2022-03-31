#pragma once

#include <grend-config.h>

#include <grend/renderFramebuffer.hpp>
#include <grend/renderSettings.hpp>
#include <grend/gameObject.hpp>
#include <grend/glManager.hpp>
#include <grend/sdlContext.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/gameModel.hpp>
#include <grend/textureAtlas.hpp>
#include <grend/quadtree.hpp>
#include <grend/camera.hpp>
#include <grend/shaderPreprocess.hpp>

#include <list>
#include <memory>
#include <tuple>

#include <stdint.h>

namespace grendx {

// per cluster, tile, whatever, maximum lights that will be evaluated per fragment
#ifndef MAX_LIGHTS
// assume clustered
#if GLSL_VERSION >= 140 /* opengl 3.1+, has uniform buffers, can transfer many more lights */
#define MAX_LIGHTS 28
#else
#define MAX_LIGHTS 8 /* opengl 2.0, need to set each uniform, very limited number of uniforms */
#endif
#endif

// clustered light array definitions
#ifndef MAX_POINT_LIGHT_OBJECTS_CLUSTERED
#define MAX_POINT_LIGHT_OBJECTS_CLUSTERED 1024
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS_CLUSTERED
#define MAX_SPOT_LIGHT_OBJECTS_CLUSTERED 1024
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS_CLUSTERED
#define MAX_DIRECTIONAL_LIGHT_OBJECTS_CLUSTERED 32
#endif

// tiled light array definitions
// XXX: so, there's a ridiculous number of bugs in the gles3 implementation 
//      of the adreno 3xx-based phone I'm testing on, one of which is that
//      multiple UBOs aren't actually usable... so, need a fallback to
//      the old style non-tiled lights array, but using one UBO
//
//      https://developer.qualcomm.com/forum/qdn-forums/software/adreno-gpu-sdk/29611
#if defined(USE_SINGLE_UBO)
#ifndef MAX_POINT_LIGHT_OBJECTS_TILED
#define MAX_POINT_LIGHT_OBJECTS_TILED MAX_LIGHTS
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS_TILED
#define MAX_SPOT_LIGHT_OBJECTS_TILED MAX_LIGHTS
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED
#define MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED 4
#endif

#else /* not defined(USE_SINGLE_UBO) */
// otherwise, tiled lighting, lots more lights available
#ifndef MAX_POINT_LIGHT_OBJECTS_TILED
#define MAX_POINT_LIGHT_OBJECTS_TILED 80
#endif

#ifndef MAX_SPOT_LIGHT_OBJECTS_TILED
#define MAX_SPOT_LIGHT_OBJECTS_TILED 24
#endif

#ifndef MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED
#define MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED 4
#endif
#endif

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
	GLfloat position[4];    // 0
	GLfloat diffuse[4];     // 16
	GLfloat direction[4];   // 32
	GLfloat shadowmap[4];   // 48
	GLfloat shadowproj[16]; // 64
	GLfloat intensity;      // 128
	GLfloat radius;         // 132
	GLfloat angle;          // 136
	GLuint  casts_shadows;  // 140, end 144
} __attribute__((packed));

struct directional_std140 {
	GLfloat position[4];     // 0
	GLfloat diffuse[4];      // 16
	GLfloat direction[4];    // 32
	GLfloat shadowmap[4];    // 48
	GLfloat shadowproj[16];  // 64
	GLfloat intensity;       // 128
	GLuint  casts_shadows;   // 132
	GLfloat padding[2];      // 136, pad to 144
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

	point_std140 upoint_lights[MAX_POINT_LIGHT_OBJECTS_TILED];
	spot_std140 uspot_lights[MAX_SPOT_LIGHT_OBJECTS_TILED];
	directional_std140 udirectional_lights[MAX_DIRECTIONAL_LIGHT_OBJECTS_TILED];

	uint8_t lol[128];
} __attribute__((packed));

// check to make sure everything will fit in the (specifications) minimum required UBO
// TODO: configurable maximum, or adjust size for the largest UBO on the platform
static_assert(sizeof(lights_std140) <= 16384,
              "Packed light attributes in lights_std140 must be <=16384 bytes!");

struct light_tiles_std140 {
	GLuint indexes[9*16*MAX_LIGHTS];
} __attribute__((packed));

static_assert(sizeof(light_tiles_std140) <= 16384,
              "Packed light tiles in light_tiles_std140 must be <=16384 bytes!");

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
		renderAtlases(unsigned refSize    = 2048,
		              unsigned shadowSize = 4096,
		              unsigned irradSize  = 1024,
		              unsigned coeffSize  = 1024)
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
	Program::ptr instancedShader;
	Program::ptr billboardShader;

	bool cull_faces = true;
	bool sort       = true;
	bool stencil    = true;
	bool depthTest  = true;
	bool depthMask  = true;
	bool syncshader = true;
	bool shadowmap  = false;
};

static inline
bool operator==(const renderFlags& lhs, const renderFlags& rhs) noexcept {
	return lhs.mainShader      == rhs.mainShader
	    && lhs.skinnedShader   == rhs.skinnedShader
	    && lhs.instancedShader == rhs.instancedShader
	    && lhs.billboardShader == rhs.billboardShader
	    && lhs.cull_faces      == rhs.cull_faces
	    && lhs.sort            == rhs.sort
	    && lhs.stencil         == rhs.stencil
	    && lhs.depthTest       == rhs.depthTest
	    && lhs.depthMask       == rhs.depthMask
	    && lhs.syncshader      == rhs.syncshader
	    && lhs.shadowmap       == rhs.shadowmap
	    ;
}


class renderContext {
	public:
		typedef std::shared_ptr<renderContext> ptr;
		typedef std::weak_ptr<renderContext> weakptr;

		enum lightingModes {
			Clustered,
			Tiled,
			PlainArray,
		};

		renderContext(context& ctx, renderSettings _settings = renderSettings());
		~renderContext() { };

		void applySettings(renderSettings& settings);
		void loadShaders(void);

		// TODO: 
		// see drawn_entities below
		void newframe(void);
		// look up stencil buffer index in drawn objects
		gameMesh::ptr index(unsigned idx);
		void setArrayMode(enum lightingModes mode);

		// XXX
		struct renderFlags getLightingFlags(std::string name="");
		void setReflectionProbe(gameReflectionProbe::ptr probe,
		                        Program::ptr program);
		void setIrradianceProbe(gameIrradianceProbe::ptr probe,
		                        Program::ptr program);

		// TODO: would be ideal to have "low", "medium", "high" or something
		//       categories, so lighting settings could be easily mapped to
		//       graphics settings
		void setDefaultLightModel(std::string name);

		// TODO: swap between these
		renderFramebuffer::ptr framebuffer;
		//renderFramebuffer last_frame;

		// (actual) screen size
		int screen_x, screen_y;

		// sky box
		skybox defaultSkybox;

		// global rendering state
		renderAtlases atlases;
		Shader::parameters globalShaderOptions;
		enum lightingModes lightingMode = lightingModes::Tiled;

		// maps 
		std::string defaultLighting = "main";
		std::map<std::string, renderFlags> lightingShaders;
		std::map<std::string, renderFlags> probeShaders;
		std::map<std::string, Program::ptr> postShaders;
		std::map<std::string, Program::ptr> internalShaders;

		Buffer::ptr lightBuffer;
		Buffer::ptr pointTiles;
		Buffer::ptr spotTiles;

		lights_std140      lightBufferCtx;
		light_tiles_std140 pointTilesCtx;
		light_tiles_std140 spotTilesCtx;

		float lightThreshold = 0.05;
		float exposure       = 1.f;
		renderSettings settings;

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
		         float animTime = 0.0,
		         glm::mat4 trans = glm::mat4(1),
		         bool inverted = false);

		void add(renderQueue& other);

		void addSkinned(gameObject::ptr obj,
		                gameSkin::ptr skin,
		                float animTime = 0.0,
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

renderFlags loadLightingShader(std::string fragmentPath, Shader::parameters& opts);
renderFlags loadProbeShader(std::string fragmentPath, Shader::parameters& opts);
Program::ptr loadPostShader(std::string fragmentPath, Shader::parameters& opts);
renderFlags loadShaderToFlags(std::string fragmentPath,
                              std::string mainVertex,
                              std::string skinnedVertex,
                              std::string instancedVertex,
                              std::string billboardVertex,
                              Shader::parameters& opts);

// TODO: should this pass transform or position?
//       sticking with transform for now
void drawShadowCubeMap(renderQueue& queue,
                       gameLightPoint::ptr light,
                       glm::mat4& transform,
					   renderContext::ptr rctx);
void drawSpotlightShadow(renderQueue& queue,
                         gameLightSpot::ptr light,
                         glm::mat4& transform,
					     renderContext::ptr rctx);
void drawReflectionProbe(renderQueue& queue,
                         gameReflectionProbe::ptr probe,
                         glm::mat4& transform,
                         renderContext::ptr rctx);
void drawIrradianceProbe(renderQueue& queue,
                         gameIrradianceProbe::ptr probe,
                         glm::mat4& transform,
                         renderContext::ptr rctx);

void buildTilemap(renderQueue::LightQ& queue, camera::ptr cam, renderContext::ptr rctx);
void buildTilemapTiled(renderQueue::LightQ& queue, camera::ptr cam, renderContext::ptr rctx);
void buildTilemapClustered(renderQueue::LightQ& queue, camera::ptr cam, renderContext::ptr rctx);

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
void set_material(Program::ptr program, compiledMaterial::ptr mesh);
void set_default_material(Program::ptr program);
void invalidateLightMaps(gameObject::ptr tree);

// namespace grendx
}

// injecting a hash function for renderflags into std namespace
template <>
struct std::hash<grendx::renderFlags> {
	std::size_t operator()(const grendx::renderFlags& r) const noexcept {
		std::size_t ret = 737;

		unsigned k = (r.cull_faces << 6)
		           | (r.sort       << 5)
		           | (r.stencil    << 4)
		           | (r.depthTest  << 3)
		           | (r.depthMask  << 2)
		           | (r.syncshader << 1)
		           | (r.shadowmap  << 0);

		ret = ret*33 + (uintptr_t)r.mainShader.get();
		ret = ret*33 + (uintptr_t)r.skinnedShader.get();
		ret = ret*33 + (uintptr_t)r.instancedShader.get();
		ret = ret*33 + (uintptr_t)r.billboardShader.get();
		ret = ret*33 + k;

		return ret;
	}
};
