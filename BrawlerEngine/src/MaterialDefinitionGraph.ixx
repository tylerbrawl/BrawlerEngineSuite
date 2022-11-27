module;
#include <concepts>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <span>
#include <vector>

export module Brawler.MaterialDefinitionGraph;
import Brawler.FilePathHash;
import Brawler.I_MaterialDefinition;
import Brawler.MaterialDefinitionHandle;

/*
An interesting thing to note is that the MaterialDefinitionGraph does not care about the
types of the scene textures; it only maps between texture FilePathHash values and the
I_MaterialDefinition instances which depend on them.
*/

export namespace Brawler
{
	class MaterialDefinitionGraph final
	{
	private:
		struct SceneTextureDependencyInfo
		{
			std::vector<const I_MaterialDefinition*> DependentMaterialDefinitionPtrArr;
			bool WasSceneTextureModifiedThisFrame;
		};

	private:
		MaterialDefinitionGraph() = default;

	public:
		~MaterialDefinitionGraph() = default;

		MaterialDefinitionGraph(const MaterialDefinitionGraph& rhs) = delete;
		MaterialDefinitionGraph& operator=(const MaterialDefinitionGraph& rhs) = delete;

		MaterialDefinitionGraph(MaterialDefinitionGraph&& rhs) noexcept = delete;
		MaterialDefinitionGraph& operator=(MaterialDefinitionGraph&& rhs) noexcept = delete;

		static MaterialDefinitionGraph& GetInstance();

		template <typename MaterialDefinitionType, typename... Args>
			requires std::derived_from<MaterialDefinitionType, I_MaterialDefinition> && std::constructible_from<MaterialDefinitionType, Args...>
		MaterialDefinitionHandle CreateMaterialDefinition(Args&&... args);

		void RequestGPUSceneMaterialDescriptorUpdate(const FilePathHash texturePathHash);

		void Update();

	private:
		std::vector<std::unique_ptr<I_MaterialDefinition>> mDefinitionPtrArr;

		/// <summary>
		/// This is a map between scene texture FilePathHash values and the I_MaterialDefinition
		/// instances which depend on them.
		/// </summary>
		std::unordered_map<FilePathHash, SceneTextureDependencyInfo> mMaterialDependencyMap;

		mutable std::mutex mCritSection;
	};
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename MaterialDefinitionType, typename... Args>
		requires std::derived_from<MaterialDefinitionType, I_MaterialDefinition> && std::constructible_from<MaterialDefinitionType, Args...>
	MaterialDefinitionHandle MaterialDefinitionGraph::CreateMaterialDefinition(Args&&... args)
	{
		std::unique_ptr<I_MaterialDefinition> materialDefinitionPtr{ std::make_unique<MaterialDefinitionType>(std::forward<Args>(args)...) };
		I_MaterialDefinition* const rawMaterialDefinitionPtr = materialDefinitionPtr.get();

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };

			{
				// For each scene texture which this material definition is dependent on, add
				// this I_MaterialDefinition instance as a dependency. This allows us to track
				// which GPUSceneBuffer MaterialDescriptor instances need to be updated when a
				// given scene texture changes.
				const std::span<const FilePathHash> dependentTexturePathHashSpan{ materialDefinitionPtr->GetDependentSceneTextureFilePathHashSpan() };

				for (const auto pathHash : dependentTexturePathHashSpan)
				{
					SceneTextureDependencyInfo& dependencyInfo{ mMaterialDependencyMap[pathHash] };
					dependencyInfo.DependentMaterialDefinitionPtrArr.push_back(rawMaterialDefinitionPtr);
				}
			}

			mDefinitionPtrArr.push_back(std::move(materialDefinitionPtr));
		}

		// Get the material definition to update its GPUScene buffer data immediately.
		rawMaterialDefinitionPtr->UpdateGPUSceneMaterialDescriptor();

		return MaterialDefinitionHandle{ *rawMaterialDefinitionPtr };
	}
}