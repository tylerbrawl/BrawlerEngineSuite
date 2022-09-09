module;
#include <optional>
#include <cassert>
#include <DirectXMath/DirectXMath.h>

module Brawler.ModelInstanceRenderComponent;
import Brawler.GPUSceneBufferID;
import Brawler.D3D12.BufferResource;
import Brawler.GPUSceneManager;
import Brawler.SceneNode;
import Brawler.TransformComponent;
import Brawler.GPUSceneBufferUpdater;
import Brawler.D3D12.BufferCopyRegion;

namespace
{
	constexpr Brawler::Math::Matrix4x3 CompressWorldMatrix(const Brawler::Math::Matrix4x4& worldMatrix)
	{
		return Brawler::Math::Matrix4x3{
			worldMatrix.GetElement(0, 0), worldMatrix.GetElement(0, 1), worldMatrix.GetElement(0, 2),
			worldMatrix.GetElement(1, 0), worldMatrix.GetElement(1, 1), worldMatrix.GetElement(1, 2),
			worldMatrix.GetElement(2, 0), worldMatrix.GetElement(2, 1), worldMatrix.GetElement(2, 2)
		};
	}
}

namespace Brawler
{
	ModelInstanceRenderComponent::ModelInstanceRenderComponent() :
		I_Component(),
		mDescriptorBufferSubAllocation(),
		mPrevFrameWorldMatrixInfo(),
		mPendingRenderDataBufferIndex(),
		mNumPendingGPUTransformUpdates(1)
	{
		D3D12::BufferResource& modelInstanceDescriptorBuffer{ GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::MODEL_INSTANCE_DESCRIPTOR_BUFFER>() };
		std::optional<D3D12::StructuredBufferSubAllocation<ModelInstanceDescriptor, 1>> descriptorBufferSubAllocation{ modelInstanceDescriptorBuffer.CreateBufferSubAllocation<D3D12::StructuredBufferSubAllocation<ModelInstanceDescriptor, 1>>() };

		assert(descriptorBufferSubAllocation.has_value() && "ERROR: The maximum number of model instances has been exceeded!");

		mDescriptorBufferSubAllocation = std::move(*descriptorBufferSubAllocation);
	}

	void ModelInstanceRenderComponent::Update(const float dt)
	{
		UpdateGPUTransformData();
		UpdateGPUDescriptorData();
	}

	void ModelInstanceRenderComponent::SetModelInstanceRenderDataBufferIndex(const std::uint32_t index)
	{
		mPendingRenderDataBufferIndex = index;
	}

	std::uint32_t ModelInstanceRenderComponent::GetModelInstanceID() const
	{
		return static_cast<std::uint32_t>(mDescriptorBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(ModelInstanceDescriptor));
	}

	void ModelInstanceRenderComponent::UpdateGPUTransformData()
	{
		// Get the TransformComponent. If it does not exist, then we are doing something wrong.
		const TransformComponent* const transformComponentPtr = GetSceneNode().GetComponent<const TransformComponent>();
		assert(transformComponentPtr != nullptr);

		if (transformComponentPtr->HasWorldMatrixChangedThisUpdate()) [[unlikely]]
			mNumPendingGPUTransformUpdates = 2;

		else if (mNumPendingGPUTransformUpdates == 0) [[likely]]
			return;
		
		const Math::Matrix4x4 currWorldMatrix{ transformComponentPtr->GetWorldMatrix() };
		const Math::Matrix4x4 currInvWorldMatrix{ currWorldMatrix.Inverse() };
		
		const WorldMatrixInfo currWorldMatrixInfo{
			.WorldMatrix{ CompressWorldMatrix(currWorldMatrix) },
			.InverseWorldMatrix{ CompressWorldMatrix(currInvWorldMatrix) }
		};

		// On the first call to ModelInstanceRenderComponent::UpdateGPUTransformData(), we don't have
		// any value stored for the previous frame's world matrix and inverse world matrix. So, we
		// just have to use the current frame's matrices.
		if (!mPrevFrameWorldMatrixInfo.has_value()) [[unlikely]]
			mPrevFrameWorldMatrixInfo = currWorldMatrixInfo;

		D3D12::BufferCopyRegion transformDataBufferCopyRegion{ D3D12::BufferCopyRegionInfo{
			.BufferResourcePtr = &(GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::MODEL_INSTANCE_TRANSFORM_DATA_BUFFER>()),
			.OffsetFromBufferStart = (GetModelInstanceID() * sizeof(ModelInstanceTransformData)),
			.RegionSizeInBytes = sizeof(ModelInstanceTransformData)
		} };
		const GPUSceneBufferUpdater<GPUSceneBufferID::MODEL_INSTANCE_TRANSFORM_DATA_BUFFER> transformDataBufferUpdater{ std::move(transformDataBufferCopyRegion) };

		ModelInstanceTransformData transformData{};

		DirectX::XMStoreFloat4x3(&(transformData.CurrentFrameWorldMatrix), currWorldMatrixInfo.WorldMatrix.GetDirectXMathMatrix());
		DirectX::XMStoreFloat4x3(&(transformData.CurrentFrameInverseWorldMatrix), currWorldMatrixInfo.InverseWorldMatrix.GetDirectXMathMatrix());
		DirectX::XMStoreFloat4x3(&(transformData.PreviousFrameWorldMatrix), mPrevFrameWorldMatrixInfo->WorldMatrix.GetDirectXMathMatrix());
		DirectX::XMStoreFloat4x3(&(transformData.PreviousFrameInverseWorldMatrix), mPrevFrameWorldMatrixInfo->InverseWorldMatrix.GetDirectXMathMatrix());

		transformDataBufferUpdater.UpdateGPUSceneData(std::move(transformData));

		mPrevFrameWorldMatrixInfo = currWorldMatrixInfo;
		--mNumPendingGPUTransformUpdates;
	}

	void ModelInstanceRenderComponent::UpdateGPUDescriptorData()
	{
		// Don't bother uploading anything to the GPU if we don't have any changes to
		// make.
		if (mPendingRenderDataBufferIndex.empty()) [[likely]]
			return;

		// In the MSVC (and presumably (?) in HLSL 2021), bit fields are packed in memory
		// such that the bit fields listed earlier in a structure appear in the less
		// significant bits of the data containing them. So, since the IsUseful field of
		// the ModelTransformData struct is listed before the __PaddingBits field (check
		// MeshTypes.hlsli), we can set the IsUseful field to 1 by also setting the
		// equivalent IsUsefulAndPadding field in the C++ version of the structure to 1.
		static constexpr std::uint32_t DESCRIPTOR_IS_USEFUL_AND_PADDING_VALUE = 1;

		const GPUSceneBufferUpdater<GPUSceneBufferID::MODEL_INSTANCE_DESCRIPTOR_BUFFER> descriptorBufferUpdater{ mDescriptorBufferSubAllocation.GetBufferCopyRegion() };
		descriptorBufferUpdater.UpdateGPUSceneData(ModelInstanceDescriptor{
			.RenderDataBufferIndex = *mPendingRenderDataBufferIndex,
			.IsUsefulAndPadding = DESCRIPTOR_IS_USEFUL_AND_PADDING_VALUE
		});

		mPendingRenderDataBufferIndex.reset();
	}

	void ModelInstanceRenderComponent::OnComponentRemoval()
	{
		// Upon the removal of a ModelInstanceRenderComponent instance from a SceneNode,
		// we should clear out the relevant ModelInstanceDescriptor to indicate that the
		// data in the buffer is no longer relevant.

		const GPUSceneBufferUpdater<GPUSceneBufferID::MODEL_INSTANCE_DESCRIPTOR_BUFFER> descriptorBufferUpdater{ mDescriptorBufferSubAllocation.GetBufferCopyRegion() };
		descriptorBufferUpdater.UpdateGPUSceneData(ModelInstanceDescriptor{});
	}
}