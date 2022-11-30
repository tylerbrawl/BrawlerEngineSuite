module;
#include <cassert>
#include <span>

module Brawler.ModelInstanceComponent;
import Brawler.GPUSceneManager;
import Brawler.GPUSceneBufferID;
import Brawler.D3D12.BufferResource;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.Model;
import Brawler.TransformComponent;

namespace Brawler 
{
	ModelInstanceComponent::ModelInstanceComponent(ModelHandle&& hModel) :
		I_Component(),
		mDescriptorBufferSubAllocation(),
		mTransformUpdater(),
		mHModel(std::move(hModel))
	{
		[[maybe_unused]] const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::MODEL_INSTANCE_DESCRIPTOR_BUFFER>().AssignReservation(mDescriptorBufferSubAllocation);
		assert(wasReservationSuccessful && "ERROR: An attempt to create a reservation from the PackedModelInstanceDescriptor GPU scene buffer failed!");

		UpdateGPUSceneBufferData();
	}

	ModelInstanceComponent::~ModelInstanceComponent()
	{
		// Upon destruction of this ModelInstanceComponent instance, clear the IsValid field of the
		// ModelInstanceDescriptor we were previously using.
		static constexpr GPUSceneTypes::PackedModelInstanceDescriptor INVALID_PACKED_MODEL_INSTANCE_DESCRIPTOR = 0;

		GPUSceneBufferUpdateOperation<GPUSceneBufferID::MODEL_INSTANCE_DESCRIPTOR_BUFFER> descriptorUpdateOperation{ mDescriptorBufferSubAllocation.GetBufferCopyRegion() };
		descriptorUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::PackedModelInstanceDescriptor>{ &INVALID_PACKED_MODEL_INSTANCE_DESCRIPTOR, 1 });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(descriptorUpdateOperation));
	}

	void ModelInstanceComponent::Update(const float dt)
	{
		const TransformComponent* const transformComponentPtr = GetSceneNode().GetComponent<const TransformComponent>();
		assert(transformComponentPtr != nullptr && "ERROR: A SceneNode was assigned a ModelInstanceComponent without ever having been assigned a TransformComponent!");

		mTransformUpdater.Update(*transformComponentPtr);
	}

	void ModelInstanceComponent::UpdateGPUSceneBufferData() const
	{
		const GPUSceneTypes::PackedModelInstanceDescriptor packedDescriptor = GetPackedModelInstanceDescriptor();

		GPUSceneBufferUpdateOperation<GPUSceneBufferID::MODEL_INSTANCE_DESCRIPTOR_BUFFER> descriptorUpdateOperation{ mDescriptorBufferSubAllocation.GetBufferCopyRegion() };
		descriptorUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::PackedModelInstanceDescriptor>{ &packedDescriptor, 1 });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(descriptorUpdateOperation));
	}

	GPUSceneTypes::PackedModelInstanceDescriptor ModelInstanceComponent::GetPackedModelInstanceDescriptor() const
	{
		// Here, we create a PackedModelInstanceDescriptor by packing all of the fields in a ModelInstanceDescriptor
		// (see MeshTypes.hlsli) into a single std::uint32_t. HLSL 2021 does support bit fields to make this
		// process easier, but as far as I can tell, it is unspecified how the DXC shader compiler will expect
		// these fields to be laid out in memory. So, we manually pack the value into a std::uint32_t.

		// Start with the value set to 1 to set the "IsValid" bit in the unpacked ModelInstanceDescriptor.
		GPUSceneTypes::PackedModelInstanceDescriptor packedDescriptor = 1;

		const std::uint32_t meshDescriptorBufferSRVIndex = mHModel.GetModel().GetMeshDescriptorBufferBindlessSRVIndex();

		// 15 bits are made available for the MeshDescriptorBufferID. If we need more, then we can always just
		// stop packing ModelInstanceDescriptor data into std::uint32_t values.
		static constexpr std::uint32_t HIGHEST_SUPPORTED_MESH_DESCRIPTOR_BUFFER_SRV_INDEX = ((1 << 15) - 1);
		assert(meshDescriptorBufferSRVIndex <= HIGHEST_SUPPORTED_MESH_DESCRIPTOR_BUFFER_SRV_INDEX && "ERROR: The maximum number of supported MeshDescriptor buffers has been exceeded! (Each Model has its own MeshDescriptor buffer, so if this limit is reached, then the maximum number of supported Models has been exceeded.)");

		packedDescriptor |= (meshDescriptorBufferSRVIndex << 1);

		const std::uint32_t transformDataBufferIndex = mTransformUpdater.GetModelInstanceTransformDataBufferIndex();

		// 16 bits are made available for the TransformDataBufferIndex. This matches the value of MAX_MODEL_INSTANCES
		// set in GPUSceneLimits.hlsli.
		static constexpr std::uint32_t HIGHEST_SUPPORTED_TRANSFORM_DATA_BUFFER_INDEX = ((1 << 16) - 1);
		assert(transformDataBufferIndex <= HIGHEST_SUPPORTED_TRANSFORM_DATA_BUFFER_INDEX && "ERROR: The maximum number of supported ModelInstanceTransformData instances has been exceeded! (This should never happen unless the value of MAX_MODEL_INSTANCES in GPUSceneLimits.hlsli changed without updating the PackedModelInstanceDescriptor layout.)");

		packedDescriptor |= (transformDataBufferIndex << 16);

		return packedDescriptor;
	}
}