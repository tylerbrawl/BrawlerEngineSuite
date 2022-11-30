module;
#include <cstdint>

export module Brawler.ModelInstanceComponent:ModelInstanceTransformUpdater;
import Brawler.TransformComponent;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class ModelInstanceTransformUpdater
	{
	public:
		ModelInstanceTransformUpdater();

		ModelInstanceTransformUpdater(const ModelInstanceTransformUpdater& rhs) = delete;
		ModelInstanceTransformUpdater& operator=(const ModelInstanceTransformUpdater& rhs) = delete;

		ModelInstanceTransformUpdater(ModelInstanceTransformUpdater&& rhs) noexcept = default;
		ModelInstanceTransformUpdater& operator=(ModelInstanceTransformUpdater&& rhs) noexcept = default;

		void Update(const TransformComponent& transformComponent);

		std::uint32_t GetModelInstanceTransformDataBufferIndex() const;

	private:
		void UpdateGPUSceneBufferData(const Math::Float4x4& currentFrameWorldMatrix) const;

		void ResetUpdateCount();

	private:
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::ModelInstanceTransformData> mTransformDataBufferSubAllocation;
		Math::Float4x3 mPreviousFrameWorldMatrix;
		Math::Float4x3 mPreviousFrameInverseWorldMatrix;
		std::uint32_t mNumUpdatesRemaining;
		bool mIsPreviousFrameDataValid;
	};
}