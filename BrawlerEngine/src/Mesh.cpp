module;

module Brawler.Mesh;

namespace Brawler
{
	Mesh::Mesh(MeshInfo&& meshInfo) :
		mInfo(std::move(meshInfo))
	{}

	GPUSceneTypes::MeshDescriptor Mesh::GetMeshDescriptor() const
	{
		return GPUSceneTypes::MeshDescriptor{
			.CurrentFrameAABBMin{ mInfo.MinimumAABBPoint.GetX(), mInfo.MinimumAABBPoint.GetY(), mInfo.MinimumAABBPoint.GetZ() },
			.MaterialDescriptorIndex = mInfo.HMaterial.GetGPUSceneBufferIndex(),
			.CurrentFrameAABBMax{ mInfo.MaximumAABBPoint.GetX(), mInfo.MaximumAABBPoint.GetY(), mInfo.MaximumAABBPoint.GetZ() },
			.IndexBufferSRVIndex = mInfo.MeshIndexBuffer.GetBindlessSRVIndex()
		};
	}
}