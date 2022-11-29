module;
#include <memory>
#include <span>
#include <vector>

export module Brawler.Model;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.BindlessSRVAllocation;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.Mesh;
import Brawler.GPUSceneTypes;

/*
Each Model is represented by a StructuredBuffer<MeshDescriptor>. Each MeshDescriptor has
some AABBs, a MaterialDescriptor, and an index buffer. Each index buffer identifies a vertex
within the global vertex buffer.

A Model class instance* owns all of the Mesh instances which comprise it. This includes any relevant
CPU and GPU data. Typically, but not always, a Model will be owned by the single ModelDatabase
instance. However, it is possible for another class to own/manage a Model class instance, and
doing this might make sense in some scenarios. For example, each model instance of a skinned mesh
is going to need a unique set of vertices in the global vertex buffer. In that case, it might
make sense for the SceneNode using a skinned mesh to own the Model class instance.

*NOTE: The term "Model class instance" is not to be confused with ModelInstance or just model
instance, which refers to a SceneNode using a given model during rendering. When we refer to
a Model class instance, we are speaking specifically about an instance of the Brawler::Model
class in the object-oriented programming sense.
*/

export namespace Brawler 
{
	class Model
	{
	public:
		Model() = default;
		explicit Model(const std::span<Mesh> meshSpan);

		Model(const Model& rhs) = delete;
		Model& operator=(const Model& rhs) = delete;

		Model(Model&& rhs) noexcept = default;
		Model& operator=(Model&& rhs) noexcept = default;

		std::uint32_t GetMeshDescriptorBufferBindlessSRVIndex() const;

	private:
		void UpdateMeshDescriptorBufferData();

	private:
		std::unique_ptr<D3D12::BufferResource> mMeshDescriptorBufferPtr;
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::MeshDescriptor> mMeshDescriptorBufferSubAllocation;
		D3D12::BindlessSRVAllocation mMeshDescriptorBufferBindlessAllocation;
		std::vector<Mesh> mMeshArr;
	};
}