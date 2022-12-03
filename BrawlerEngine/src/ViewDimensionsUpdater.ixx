module;
#include <cstdint>

export module Brawler.ViewComponent:ViewDimensionsUpdater;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class ViewDimensionsUpdater
	{
	public:
		ViewDimensionsUpdater();

		ViewDimensionsUpdater(const ViewDimensionsUpdater& rhs) = delete;
		ViewDimensionsUpdater& operator=(const ViewDimensionsUpdater& rhs) = delete;

		ViewDimensionsUpdater(ViewDimensionsUpdater&& rhs) noexcept = default;
		ViewDimensionsUpdater& operator=(ViewDimensionsUpdater&& rhs) noexcept = default;

		void CheckForGPUSceneBufferUpdate(const Math::UInt2 viewDimensions);
		void ResetUpdateCounter();

		std::uint32_t GetViewDimensionsDataBufferIndex() const;

	private:
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::ViewDimensionsData> mViewDimensionsBufferSubAllocation;
		bool mNeedsUpdate;
	};
}