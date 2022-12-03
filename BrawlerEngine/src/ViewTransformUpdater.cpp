module;
#include <cassert>
#include <span>
#include <DirectXMath/DirectXMath.h>

module Brawler.ViewComponent;
import Brawler.GPUSceneManager;
import Brawler.GPUSceneBufferID;
import Brawler.D3D12.BufferResource();
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;

namespace
{
	DirectX::XMFLOAT4X4 ConvertMatrix4x4(const Brawler::Math::Float4x4& srcMatrix)
	{
		return DirectX::XMFLOAT4X4{
			srcMatrix.GetElement(0, 0), srcMatrix.GetElement(0, 1), srcMatrix.GetElement(0, 2), srcMatrix.GetElement(0, 3),
			srcMatrix.GetElement(1, 0), srcMatrix.GetElement(1, 1), srcMatrix.GetElement(1, 2), srcMatrix.GetElement(1, 3),
			srcMatrix.GetElement(2, 0), srcMatrix.GetElement(2, 1), srcMatrix.GetElement(2, 2), srcMatrix.GetElement(2, 3),
			srcMatrix.GetElement(3, 0), srcMatrix.GetElement(3, 1), srcMatrix.GetElement(3, 2), srcMatrix.GetElement(3, 3)
		};
	}

	DirectX::XMFLOAT4 ConvertQuaternion(const Brawler::Math::Quaternion& srcQuaternion)
	{
		const Brawler::Math::Float3 imaginaryPart{ srcQuaternion.GetVectorComponent() };

		return DirectX::XMFLOAT4{ imaginaryPart.GetX(), imaginaryPart.GetY(), imaginaryPart.GetZ(), srcQuaternion.GetScalarComponent() };
	}

	DirectX::XMFLOAT3 ConvertFloat3(const Brawler::Math::Float3& srcVector)
	{
		return DirectX::XMFLOAT3{ srcVector.GetX(), srcVector.GetY(), srcVector.GetZ() };
	}
}

namespace Brawler
{
	ViewTransformUpdater::ViewTransformUpdater(const ViewTransformInfo& transformInfo) :
		mViewTransformBufferSubAllocation(),
		mPrevFrameViewProjectionMatrix(transformInfo.ViewProjectionMatrix),
		mPrevFrameInverseViewProjectionMatrix(transformInfo.InverseViewProjectionMatrix),
		mPrevFrameViewSpaceQuaternion(transformInfo.ViewSpaceQuaternion),
		mPrevFrameWorldSpaceOriginVS(transformInfo.WorldSpaceOriginVS),
		mNumUpdatesRemaining(1)
	{
		[[maybe_unused]] const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::VIEW_TRANSFORM_DATA_BUFFER>().AssignReservation(mViewTransformBufferSubAllocation);
		assert(wasReservationSuccessful && "ERROR: An attempt to assign a reservation from the ViewTransformData GPU scene buffer failed!");
	}

	void ViewTransformUpdater::SetPreviousFrameTransformData(const ViewTransformInfo& prevFrameTransformInfo)
	{
		mPrevFrameViewProjectionMatrix = prevFrameTransformInfo.ViewProjectionMatrix;
		mPrevFrameInverseViewProjectionMatrix = prevFrameTransformInfo.InverseViewProjectionMatrix;
		mPrevFrameViewSpaceQuaternion = prevFrameTransformInfo.ViewSpaceQuaternion;
		mPrevFrameWorldSpaceOriginVS = prevFrameTransformInfo.WorldSpaceOriginVS;
	}

	void ViewTransformUpdater::CheckForGPUSceneBufferUpdate(const ViewTransformInfo& currFrameTransformInfo)
	{
		if (mNumUpdatesRemaining > 0) [[unlikely]]
		{
			// Update the data in the GPU scene buffer with the current ViewTransformData.
			const GPUSceneTypes::ViewTransformData currViewTransformData{
				.CurrentFrameViewProjectionMatrix{ ConvertMatrix4x4(currFrameTransformInfo.ViewProjectionMatrix) },
				.CurrentFrameInverseViewProjectionMatrix{ ConvertMatrix4x4(currFrameTransformInfo.InverseViewProjectionMatrix) },
				.PreviousFrameViewProjectionMatrix{ ConvertMatrix4x4(mPrevFrameViewProjectionMatrix) },
				.PreviousFrameInverseViewProjectionMatrix{ ConvertMatrix4x4(mPrevFrameInverseViewProjectionMatrix) },
				.CurrentFrameViewSpaceQuaternion{ ConvertQuaternion(currFrameTransformInfo.ViewSpaceQuaternion) },
				.CurrentFrameWorldSpaceOriginVS{ ConvertFloat3(currFrameTransformInfo.WorldSpaceOriginVS) },
				.PreviousFrameViewSpaceQuaternion{ ConvertQuaternion(mPrevFrameViewSpaceQuaternion) },
				.PreviousFrameWorldSpaceOriginVS{ mPrevFrameWorldSpaceOriginVS }
			};

			GPUSceneBufferUpdateOperation<GPUSceneBufferID::VIEW_TRANSFORM_DATA_BUFFER> viewTransformUpdateOperation{ mViewTransformBufferSubAllocation.GetBufferCopyRegion() };
			viewTransformUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::ViewTransformData>{ &currViewTransformData, 1 });

			Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(viewTransformUpdateOperation));

			--mNumUpdatesRemaining;
		}
	}

	void ViewTransformUpdater::ResetUpdateCounter()
	{
		// We always keep data in GPU memory for two frames: the current frame and the previous
		// frame. Having the previous frame's ViewTransformData is useful for, e.g., projecting
		// points to the previous frame's NDC space during a TAA resolve.
		static constexpr std::uint32_t TRACKED_FRAME_COUNT = 2;

		mNumUpdatesRemaining = TRACKED_FRAME_COUNT;
	}

	std::uint32_t ViewTransformUpdater::GetViewTransformDataBufferIndex() const
	{
		return (mViewTransformBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(GPUSceneTypes::ViewTransformData));
	}
}