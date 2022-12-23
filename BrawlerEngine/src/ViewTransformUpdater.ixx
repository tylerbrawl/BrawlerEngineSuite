module;
#include <cstdint>

export module Brawler.ViewComponent:ViewTransformUpdater;
import Brawler.GPUSceneTypes;
import Brawler.Math.MathTypes;
import Brawler.D3D12.StructuredBufferSubAllocation;

export namespace Brawler 
{
	struct ViewTransformInfo
	{
		const Math::Float4x4& ViewProjectionMatrix;
		const Math::Float4x4& InverseViewProjectionMatrix;
		const Math::Quaternion& ViewSpaceQuaternion;
		const Math::Float3 ViewSpaceOriginWS;
	};

	class ViewTransformUpdater
	{
	public:
		explicit ViewTransformUpdater(const ViewTransformInfo& transformInfo);

		ViewTransformUpdater(const ViewTransformUpdater& rhs) = delete;
		ViewTransformUpdater& operator=(const ViewTransformUpdater& rhs) = delete;

		ViewTransformUpdater(ViewTransformUpdater&& rhs) noexcept = default;
		ViewTransformUpdater& operator=(ViewTransformUpdater&& rhs) noexcept = default;

		void SetPreviousFrameTransformData(const ViewTransformInfo& prevFrameTransformInfo);

		void CheckForGPUSceneBufferUpdate(const ViewTransformInfo& currFrameTransformInfo);
		void ResetUpdateCounter();

		std::uint32_t GetViewTransformDataBufferIndex() const;

	private:
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::ViewTransformData> mViewTransformBufferSubAllocation;
		Math::Float4x4 mPrevFrameViewProjectionMatrix;
		Math::Float4x4 mPrevFrameInverseViewProjectionMatrix;
		Math::Quaternion mPrevFrameViewSpaceQuaternion;
		Math::Float3 mPrevFrameViewSpaceOriginWS;
		std::uint32_t mNumUpdatesRemaining;
	};
}