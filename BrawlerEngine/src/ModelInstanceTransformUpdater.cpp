module;
#include <cassert>
#include <cstdint>
#include <span>
#include <DirectXMath/DirectXMath.h>

module Brawler.ModelInstanceComponent;
import Brawler.GPUSceneManager;
import Brawler.D3D12.BufferResource;
import Brawler.GPUSceneBufferID;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdaterRenderModule;
import Brawler.GPUSceneBufferUpdateOperation;

namespace
{
	DirectX::XMFLOAT4x3 CondenseFloat4x4(const Math::Float4x4& srcMatrix)
	{
		return DirectX::XMFLOAT4x3{
			srcMatrix.GetElement(0, 0), srcMatrix.GetElement(0, 1), srcMatrix.GetElement(0, 2),
			srcMatrix.GetElement(1, 0), srcMatrix.GetElement(1, 1), srcMatrix.GetElement(1, 2),
			srcMatrix.GetElement(2, 0), srcMatrix.GetElement(2, 1), srcMatrix.GetElement(2, 2),
			srcMatrix.GetElement(3, 0), srcMatrix.GetElement(3, 1), srcMatrix.GetElement(3, 2)
		};
	}
}

namespace Brawler
{
	ModelInstanceTransformUpdater::ModelInstanceTransformUpdater() :
		mTransformDataBufferSubAllocation(),
		mPreviousFrameWorldMatrix(),
		mPreviousFrameInverseWorldMatrix(),
		mNumUpdatesRemaining(),
		mIsPreviousFrameDataValid(false)
	{
		[[maybe_unused]] const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::MODEL_INSTANCE_TRANSFORM_DATA_BUFFER>().AssignReservation(mTransformDataBufferSubAllocation);
		assert(wasReservationSuccessful && "ERROR: An attempt to create a reservation from the ModelInstanceTransformData GPU scene buffer failed!");

		ResetUpdateCount();
	}

	void ModelInstanceTransformUpdater::Update(const TransformComponent& transformComponent)
	{
		// On the first call to ModelInstanceTransformData::Update(), set the "previous" frame's
		// world matrix to that of the current frame's.
		if (!mIsPreviousFrameDataValid) [[unlikely]]
		{
			mPreviousFrameWorldMatrix = transformComponent.GetWorldMatrix();
			mPreviousFrameInverseWorldMatrix = mPreviousFrameWorldMatrix.Inverse();

			mIsPreviousFrameDataValid = true;
		}

		// If the world matrix has changed this frame for the relevant SceneNode, then we
		// should reset the required update count.
		if (transformComponent.HasWorldMatrixChangedThisUpdate())
			ResetUpdateCount();

		// Finally, if we have any remaining updates, then we need to modify the GPU scene buffer
		// data to account for them.
		if (mNumUpdatesRemaining > 0)
		{
			UpdateGPUSceneBufferData(transformComponent.GetWorldMatrix());
			--mNumUpdatesRemaining;
		}
	}

	std::uint32_t ModelInstanceTransformUpdater::GetModelInstanceTransformDataBufferIndex() const
	{
		return (mTransformDataBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(GPUSceneTypes::ModelInstanceTransformData));
	}

	void ModelInstanceTransformUpdater::UpdateGPUSceneBufferData(const Math::Float4x4& currentFrameWorldMatrix) const
	{
		const Math::Float4x4 currentFrameInverseWorldMatrix{ currentFrameWorldMatrix.Inverse() };
		const GPUSceneTypes::ModelInstanceTransformData currTransformData{
			.CurrentFrameWorldMatrix{ CondenseFloat4x4(currentFrameWorldMatrix) },
			.CurrentFrameInverseWorldMatrix{ CondenseFloat4x4(currentFrameInverseWorldMatrix) },
			.PreviousFrameWorldMatrix{ CondenseFloat4x4(mPreviousFrameWorldMatrix) },
			.PreviousFrameInverseWorldMatrix{ CondenseFloat4x4(mPreviousFrameInverseWorldMatrix) }
		};

		GPUSceneBufferUpdateOperation<GPUSceneBufferID::MODEL_INSTANCE_TRANSFORM_DATA_BUFFER> transformDataUpdateOperation{ mTransformDataBufferSubAllocation.GetBufferCopyRegion() };
		transformDataUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::ModelInstanceTransformData>{ &currTransformData, 1 });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(transformDataUpdateOperation));
	}

	void ModelInstanceTransformUpdater::ResetUpdateCount()
	{
		static constexpr std::uint32_t NUM_TRACKED_WORLD_MATRICES = 2;
		
		mNumUpdatesRemaining = NUM_TRACKED_WORLD_MATRICES;
	}
}