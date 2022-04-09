#pragma once

#include <grend-config.h>

#include <grend/renderQueue.hpp>
#include <grend/renderContext.hpp>
#include <grend/renderFramebuffer.hpp>
#include <grend/renderFlags.hpp>
#include <grend/renderData.hpp>

#include <grend/gameObject.hpp>
#include <grend/glManager.hpp>
#include <grend/sdlContext.hpp>
#include <grend/glmIncludes.hpp>
#include <grend/gameModel.hpp>
#include <grend/quadtree.hpp>
#include <grend/camera.hpp>
#include <grend/shaderPreprocess.hpp>

#include <list>
#include <memory>
#include <tuple>

#include <stdint.h>

namespace grendx {

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
