module;
#include <cassert>
#include <memory>
#include <span>
#include <vector>
#include <DxDef.h>

module Brawler.Model;
import Brawler.D3D12.GPUResourceViews;
import Brawler.GenericPreFrameBufferUpdate;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;

namespace Brawler
{
	Model::Model(const std::span<Mesh> meshSpan) :
		mMeshDescriptorBufferPtr(std::make_unique<D3D12::BufferResource>(D3D12::BufferResourceInitializationInfo{
			.SizeInBytes = (meshSpan.size() * sizeof(GPUSceneTypes::MeshDescriptor)),
			.HeapType = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT
		})),
		mMeshDescriptorBufferSubAllocation(meshSpan.size()),
		mMeshDescriptorBufferBindlessAllocation(),
		mMeshArr()
	{
		mMeshArr.reserve(meshSpan.size());

		for (auto&& mesh : meshSpan)
			mMeshArr.push_back(std::move(mesh));

		{
			[[maybe_unused]] const bool wasReservationSuccessful = mMeshDescriptorBufferPtr->AssignReservation(mMeshDescriptorBufferSubAllocation);
			assert(wasReservationSuccessful);
		}

		mMeshDescriptorBufferBindlessAllocation = mMeshDescriptorBufferPtr->CreateBindlessSRV(mMeshDescriptorBufferSubAllocation.CreateTableShaderResourceView());

		UpdateMeshDescriptorBufferData();
	}

	std::uint32_t Model::GetMeshDescriptorBufferBindlessSRVIndex() const
	{
		assert(mMeshDescriptorBufferPtr != nullptr);
		return mMeshDescriptorBufferBindlessAllocation.GetBindlessSRVIndex();
	}

	void Model::UpdateMeshDescriptorBufferData()
	{
		std::vector<GPUSceneTypes::MeshDescriptor> meshDescriptorArr{};
		meshDescriptorArr.reserve(mMeshArr.size());

		for (const auto& mesh : mMeshArr)
			meshDescriptorArr.push_back(mesh.GetMeshDescriptor());

		GenericPreFrameBufferUpdate meshDescriptorBufferUpdate{ GenericPreFrameBufferUpdateInfo{
			.DestBufferResourcePtr = mMeshDescriptorBufferPtr.get(),
			.OffsetFromBufferStart = mMeshDescriptorBufferSubAllocation.GetOffsetFromBufferStart(),
			.UpdateRegionSizeInBytes = mMeshDescriptorBufferSubAllocation.GetSubAllocationSize()
		} };

		meshDescriptorBufferUpdate.SetInputData(std::span<const GPUSceneTypes::MeshDescriptor>{ meshDescriptorArr });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGenericBufferUpdateForNextFrame(std::move(meshDescriptorBufferUpdate));
	}
}