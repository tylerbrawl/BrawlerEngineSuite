module;
#include <optional>

export module Brawler.ModelInstanceRenderComponent;
import Brawler.I_Component;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.Math.MathTypes;

export namespace Brawler
{
	class ModelInstanceRenderComponent final : public I_Component
	{
	private:
		struct WorldMatrixInfo
		{
			Math::Float4x3 WorldMatrix;
			Math::Float4x3 InverseWorldMatrix;
		};

	public:
		ModelInstanceRenderComponent();

		~ModelInstanceRenderComponent();

		ModelInstanceRenderComponent(const ModelInstanceRenderComponent& rhs) = delete;
		ModelInstanceRenderComponent& operator=(const ModelInstanceRenderComponent& rhs) = delete;

		ModelInstanceRenderComponent(ModelInstanceRenderComponent&& rhs) noexcept = default;
		ModelInstanceRenderComponent& operator=(ModelInstanceRenderComponent&& rhs) noexcept = default;

		void Update(const float dt) override;

		std::uint32_t GetModelInstanceID() const;

	private:
		void UpdateGPUTransformData();

	private:
		D3D12::StructuredBufferSubAllocation<ModelInstanceDescriptor, 1> mDescriptorBufferSubAllocation;
		std::optional<WorldMatrixInfo> mPrevFrameWorldMatrixInfo;
		std::uint32_t mNumPendingGPUTransformUpdates;
	};
}