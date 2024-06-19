#include <grend/gameMain.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/editor.hpp>
#include <grend/ecs/serializeDefs.hpp>

#include <grend/ecs/sceneComponent.hpp>
#include <grend/ecs/collision.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/shader.hpp>

#include <grend/ecs/ref.hpp>
#include <grend/ecs/link.hpp>

#include <grend/ecs/animationController.hpp>
#include <grend/ecs/materialComponent.hpp>
#include <grend/ecs/bufferComponent.hpp>

using namespace grendx;
using namespace grendx::ecs;
using namespace grendx::engine;

void grendx::ecs::addDefaultFactories(void) {
	auto factories = Resolve<ecs::serializer>();

	factories->add<entity>();
	factories->add<sceneComponent>();
	factories->add<rigidBody>();
	factories->add<rigidBodySphere>();
	factories->add<rigidBodyStaticMesh>();
	factories->add<rigidBodyCapsule>();
	factories->add<rigidBodyBox>();
	factories->add<rigidBodyCylinder>();

	factories->add<PBRShader>();
	factories->add<UnlitShader>();

	factories->add<sceneNode>();
	factories->add<sceneSkin>();
	factories->add<sceneParticles>();
	factories->add<sceneBillboardParticles>();
	factories->add<sceneLight>();
	factories->add<sceneLightPoint>();
	factories->add<sceneLightSpot>();
	factories->add<sceneLightDirectional>();
	factories->add<sceneReflectionProbe>();
	factories->add<sceneIrradianceProbe>();

	factories->add<sceneModel>();
	factories->add<sceneMesh>();

	factories->add<animationController>();
	factories->add<materialComponent>();

	factories->add<bufferComponent<sceneModel::vertex>>();
	factories->add<bufferComponent<sceneModel::jointWeights>>();
	factories->add<bufferComponent<sceneMesh::faceType>>();
}

void grendx::ecs::addDefaultEditors(void) {
	auto editor = Resolve<ecs::editor>();

	editor->add<ecs::entity>();
	editor->add<ecs::rigidBody>();
	editor->add<ecs::rigidBodySphere>();
	editor->add<ecs::rigidBodyStaticMesh>();
	editor->add<ecs::rigidBodyCapsule>();
	editor->add<ecs::rigidBodyBox>();
	editor->add<ecs::rigidBodyCylinder>();
	editor->add<ecs::sceneComponent>();

	editor->add<sceneNode>();
	editor->add<sceneParticles>();
	editor->add<sceneBillboardParticles>();
	editor->add<sceneLight>();
	editor->add<sceneLightPoint>();
	editor->add<sceneLightSpot>();
	editor->add<sceneLightDirectional>();
	editor->add<sceneReflectionProbe>();
	editor->add<sceneIrradianceProbe>();

	editor->add<animationController>();
	editor->add<materialComponent>();

	editor->add<bufferComponent<sceneModel::vertex>>();
	editor->add<bufferComponent<sceneModel::jointWeights>>();
	editor->add<bufferComponent<sceneMesh::faceType>>();
}
