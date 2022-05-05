module;
#include <vector>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <cassert>
#include <assimp/scene.h>

module Brawler.MaterialManager;
import Brawler.JobSystem;
import Brawler.OpaqueMaterialDefinition;
import Util.Coroutine;
import Util.General;

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
	void MaterialManager::BeginInitialization()
	{
		const aiScene& scene{ Util::General::GetScene() };
		
		mMaterialDefinitionArr.resize(static_cast<std::size_t>(scene.mNumMaterials));

		Brawler::JobGroup materialInitializationGroup{};
		materialInitializationGroup.Reserve(static_cast<std::size_t>(scene.mNumMaterials));

		const std::span<aiMaterial*> materialSpan{ scene.mMaterials, static_cast<std::size_t>(scene.mNumMaterials) };

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