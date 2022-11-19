module;
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

export module Brawler.StandardMaterialDefinition;
export import :StandardMaterialBuilder;
import Brawler.I_MaterialDefinition;
import Brawler.D3D12.StructuredBufferSubAllocation;
import Brawler.GPUSceneTypes;
import Brawler.SceneTextureHandles;

export namespace Brawler
{
	class StandardMaterialDefinition final : public I_MaterialDefinition
	{
	private:
		struct SceneTextureHandleCollection
		{
			std::optional<SceneTexture2DHandle> HBaseColorTexture;
			std::optional<SceneTexture2DHandle> HNormalMap;
			std::optional<SceneTexture2DHandle> HRoughnessTexture;
			std::optional<SceneTexture2DHandle> HMetallicTexture;
		};

	public:
		explicit StandardMaterialDefinition(const StandardMaterialBuilder& builder);

		StandardMaterialDefinition(const StandardMaterialDefinition& rhs) = delete;
		StandardMaterialDefinition& operator=(const StandardMaterialDefinition& rhs) = delete;

		StandardMaterialDefinition(StandardMaterialDefinition&& rhs) noexcept = default;
		StandardMaterialDefinition& operator=(StandardMaterialDefinition&& rhs) noexcept = default;

		std::span<const FilePathHash> GetDependentSceneTextureFilePathHashSpan() const override;
		void UpdateGPUSceneMaterialDescriptor() const override;
		std::uint32_t GetGPUSceneBufferIndex() const override;

	private:
		void InitializeSceneTextureHandles(const StandardMaterialBuilder& builder);

	private:
		D3D12::StructuredBufferSubAllocation<GPUSceneTypes::MaterialDescriptor> mMaterialDescriptorSubAllocation;
		std::vector<FilePathHash> mDependentTextureHashArr;
		SceneTextureHandleCollection mHandleCollection;
	};
}