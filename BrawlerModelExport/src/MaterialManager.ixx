module;
#include <memory>
#include <atomic>
#include <vector>
#include <assimp/scene.h>

export module Brawler.MaterialManager;
import Brawler.I_MaterialDefinition;

export namespace Brawler
{
	class MaterialManager
	{
	public:
		MaterialManager() = default;

		MaterialManager(const MaterialManager& rhs) = delete;
		MaterialManager& operator=(const MaterialManager& rhs) = delete;

		MaterialManager(MaterialManager&& rhs) noexcept = default;
		MaterialManager& operator=(MaterialManager&& rhs) noexcept = default;

		void BeginInitialization();

	private:
		std::vector<std::unique_ptr<I_MaterialDefinition>> mMaterialDefinitionArr;
	};
}