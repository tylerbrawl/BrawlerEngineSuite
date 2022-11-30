module;

export module Brawler.ModelInstanceComponent;
import :ModelInstanceTransformUpdater;
import Brawler.I_Component;
import Brawler.ModelHandle;
import Brawler.GPUSceneTypes;
import Brawler.D3D12.AlignedByteAddressBufferSubAllocation;

export namespace Brawler 
{
	class ModelInstanceComponent final : public I_Component
	{
	private:
		using SubAllocationType = D3D12::AlignedByteAddressBufferSubAllocation<sizeof(GPUSceneTypes::PackedModelInstanceDescriptor), alignof(GPUSceneTypes::PackedModelInstanceDescriptor)>;

	public:
		explicit ModelInstanceComponent(ModelHandle&& hModel);

		~ModelInstanceComponent();

		ModelInstanceComponent(const ModelInstanceComponent& rhs) = delete;
		ModelInstanceComponent& operator=(const ModelInstanceComponent& rhs) = delete;

		ModelInstanceComponent(ModelInstanceComponent&& rhs) noexcept = default;
		ModelInstanceComponent& operator=(ModelInstanceComponent&& rhs) noexcept = default;

		void Update(const float dt) override;

	private:
		void UpdateGPUSceneBufferData() const;
		GPUSceneTypes::PackedModelInstanceDescriptor GetPackedModelInstanceDescriptor() const;

	private:
		SubAllocationType mDescriptorBufferSubAllocation;
		ModelInstanceTransformUpdater mTransformUpdater;
		ModelHandle mHModel;
	};
}