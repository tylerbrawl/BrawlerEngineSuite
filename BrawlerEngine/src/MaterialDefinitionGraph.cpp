module;
#include <concepts>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <span>
#include <vector>
#include <unordered_set>

module Brawler.MaterialDefinitionGraph;

namespace Brawler
{
	MaterialDefinitionGraph& MaterialDefinitionGraph::GetInstance()
	{
		static MaterialDefinitionGraph instance{};
		return instance;
	}

	void MaterialDefinitionGraph::RequestGPUSceneMaterialDescriptorUpdate(const FilePathHash texturePathHash)
	{
		const std::scoped_lock<std::mutex> lock{ mCritSection };
		
		if (!mMaterialDependencyMap.contains(texturePathHash)) [[unlikely]]
			return;

		mMaterialDependencyMap.at(texturePathHash).WasSceneTextureModifiedThisFrame = true;
	}

	void MaterialDefinitionGraph::Update()
	{
		// We want to call I_MaterialDefinition::UpdateGPUSceneMaterialDescriptor() at most once per frame
		// per I_MaterialDefinition instance.
		std::unordered_set<const I_MaterialDefinition*> definitionsToUpdateSet{};

		// We also want to delete any I_MaterialDefinition instances which have been notified that their
		// handle has been destroyed.
		std::unordered_set<const I_MaterialDefinition*> definitionsToDeleteSet{};

		{
			const std::scoped_lock<std::mutex> lock{ mCritSection };

			for (auto& [texturePathHash, dependencyInfo] : mMaterialDependencyMap)
			{
				// Mark any relevant I_MaterialDefinition instances for deletion.
				std::erase_if(dependencyInfo.DependentMaterialDefinitionPtrArr, [&definitionsToDeleteSet] (const I_MaterialDefinition* const currDefinitionPtr)
				{
					if (!currDefinitionPtr->ReadyForDeletion()) [[likely]]
						return false;

					definitionsToDeleteSet.insert(currDefinitionPtr);
					return true;
				});
				
				// Skip this scene texture if no updates took place.
				if (!dependencyInfo.WasSceneTextureModifiedThisFrame) [[likely]]
					continue;

				dependencyInfo.WasSceneTextureModifiedThisFrame = false;

				// Otherwise, add every dependent I_MaterialDefinition instance
				// as one which needs an update.
				for (const auto definitionPtr : dependencyInfo.DependentMaterialDefinitionPtrArr)
					definitionsToUpdateSet.insert(definitionPtr);
			}

			// Before we release the lock, delete all I_MaterialDefinition instances which are
			// no longer needed.
			std::erase_if(mDefinitionPtrArr, [&definitionsToDeleteSet] (const std::unique_ptr<I_MaterialDefinition>& currDefinitionPtr) { return definitionsToDeleteSet.contains(currDefinitionPtr.get()); });
		}

		// We don't need to acquire the lock to update the GPUSceneBuffer data.
		for (const auto definitionPtr : definitionsToUpdateSet)
			definitionPtr->UpdateGPUSceneMaterialDescriptor();
	}
}