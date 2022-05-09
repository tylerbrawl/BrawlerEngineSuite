module;
#include <vector>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <cassert>
#include <atomic>
#include <assimp/scene.h>

module Brawler.MaterialManager;
import Brawler.JobSystem;
import Brawler.OpaqueMaterialDefinition;
import Util.Coroutine;
import Util.General;
import Util.ModelExport;

namespace
{
	std::unique_ptr<Brawler::I_MaterialDefinition> CreateMaterialDefinition(const aiMaterial& material)
	{
		aiString materialName{};
		const aiReturn getMaterialNameReturn{ material.Get(AI_MATKEY_NAME, materialName) };
		assert(getMaterialNameReturn == aiReturn::aiReturn_SUCCESS);
		
		// For now, we only support opaque materials.

		float materialOpacity = 0.0f;
		const aiReturn getMaterialOpacityReturn{ material.Get(AI_MATKEY_OPACITY, materialOpacity) };
		assert(getMaterialOpacityReturn == aiReturn::aiReturn_SUCCESS);

		if (materialOpacity != 1.0f)
			throw std::runtime_error{ std::string{"ERROR: The material "} + materialName.C_Str() + " is transparent! (Currently, transparent/translucent materials are not supported.)" };

		return std::make_unique<Brawler::OpaqueMaterialDefinition>(material);
	}
}

namespace Brawler
{
	MaterialManager::MaterialManager() :
		mMaterialDefinitionArr(),
		mActiveSerializationCount(0)
	{
		CreateSerializationJobs();
	}

	void MaterialManager::CreateSerializationJobs()
	{
		const aiScene& scene{ Util::ModelExport::GetScene() };
		
		const std::size_t materialCount = static_cast<std::size_t>(scene.mNumMaterials);
		mMaterialDefinitionArr.resize(materialCount);

		mActiveSerializationCount.store(scene.mNumMaterials, std::memory_order::relaxed);

		Brawler::JobGroup materialInitializationGroup{};
		materialInitializationGroup.Reserve(materialCount);

		const std::span<aiMaterial*> materialSpan{ scene.mMaterials, materialCount };

		// We don't have std::views::zip yet, so we'll just have to do this.
		for (std::size_t i = 0; i < materialSpan.size(); ++i)
		{
			std::unique_ptr<I_MaterialDefinition>& materialDefinitionPtr{ mMaterialDefinitionArr[i] };
			materialInitializationGroup.AddJob([&materialDefinitionPtr, materialPtr = materialSpan[i]]()
			{
				materialDefinitionPtr = CreateMaterialDefinition(*materialPtr);
			});
		}

		materialInitializationGroup.ExecuteJobs();
	}
}