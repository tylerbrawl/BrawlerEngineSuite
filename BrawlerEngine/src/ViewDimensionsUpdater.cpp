module;
#include <cassert>
#include <span>

module Brawler.ViewComponent;
import Brawler.GPUSceneManager;
import Brawler.GPUSceneBufferID;
import Brawler.D3D12.BufferResource;
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.Application;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;

namespace Brawler
{
	ViewDimensionsUpdater::ViewDimensionsUpdater() :
		mViewDimensionsBufferSubAllocation(),
		mNeedsUpdate(true)
	{
		[[maybe_unused]] const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::VIEW_DIMENSIONS_DATA_BUFFER>().AssignReservation(mViewDimensionsBufferSubAllocation);
		assert(wasReservationSuccessful && "ERROR: An attempt to create a reservation from the ViewDimensionsData GPU scene buffer failed!");
	}

	void ViewDimensionsUpdater::CheckForGPUSceneBufferUpdate(const Math::UInt2 viewDimensions)
	{
		if (mNeedsUpdate) [[unlikely]]
		{
			const Math::Float2 inverseViewDimensions{ viewDimensions.GetReciprocal() };
			const GPUSceneTypes::ViewDimensionsData currViewDimensionsData{
				.ViewDimensions{ viewDimensions.GetX(), viewDimensions.GetY() },
				.InverseViewDimensions{ inverseViewDimensions.GetX(), inverseViewDimensions.GetY() }
			};

			GPUSceneBufferUpdateOperation<GPUSceneBufferID::VIEW_DIMENSIONS_DATA_BUFFER> viewDimensionsUpdateOperation{ mViewDimensionsBufferSubAllocation.GetBufferCopyRegion() };
			viewDimensionsUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::ViewDimensionsData>{ &currViewDimensionsData, 1 });

			Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(viewDimensionsUpdateOperation));

			mNeedsUpdate = false;
		}
	}

	void ViewDimensionsUpdater::ResetUpdateCounter()
	{
		mNeedsUpdate = true;
	}

	std::uint32_t ViewDimensionsUpdater::GetViewDimensionsDataBufferIndex() const
	{
		return (mViewDimensionsBufferSubAllocation.GetOffsetFromBufferStart() / sizeof(GPUSceneTypes::ViewDimensionsData));
	}
}