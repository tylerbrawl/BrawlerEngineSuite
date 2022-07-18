module;

export module Brawler.I_LODMeshDefinition;
import Brawler.SceneNode;

export namespace Brawler
{
	/*
	An LODMeshDefinition is responsible for managing all of the data of an LOD mesh which is universally
	shared amongst all ModelInstances using it. (A ModelInstance is simply a SceneNode which uses an LOD
	mesh for rendering.) It shall also be responsible for loading per-ModelInstance data as needed and
	assigning it to an LODMeshInstantiation component given to the SceneNode.

	I_LODMeshDefinition is an interface which makes use of dynamic polymorphism. The reasoning behind
	this is that we want to create a consistent interface for allowing SceneNodes to swap out their LOD
	mesh used for rendering easily; this will make LOD management significantly easier. However, different
	types of LOD meshes will have different data which can and cannot be shared across ModelInstances.

	For example, consider a static LOD mesh. Its vertex buffer and index buffer elements are never going
	to change so long as it is loaded, so its data can be owned by a (hypothetical) StaticLODMeshDefinition
	instance. Now, consider a skinned LOD mesh. Each ModelInstance using this LOD mesh must have a different
	allocation within the global vertex buffer so that they can be animated uniquely. For that reason, each
	LODMeshInstantiation for a skinned LOD mesh should own its allocation within the vertex buffer, and not
	the (hypothetical) SkinnedLODMeshDefinition. The index buffer, however, *IS* still shared between the
	ModelInstances, as is any keyframe data used for animations. Those should be owned by the
	SkinnedLODMeshDefinition instance and referenced by the LODMeshInstantiation instances.
	*/

	class I_LODMeshDefinition
	{
	protected:
		I_LODMeshDefinition() = default;

	public:
		virtual ~I_LODMeshDefinition() = default;

		I_LODMeshDefinition(const I_LODMeshDefinition& rhs) = delete;
		I_LODMeshDefinition& operator=(const I_LODMeshDefinition& rhs) = delete;

		I_LODMeshDefinition(I_LODMeshDefinition&& rhs) noexcept = default;
		I_LODMeshDefinition& operator=(I_LODMeshDefinition&& rhs) noexcept = default;

		virtual void AssignLODMeshToSceneNode(SceneNode& node) = 0;
	};
}