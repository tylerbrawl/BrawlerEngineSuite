module;
#include <cstdint>

export module Brawler.MaterialDefinitionHandle;
import Brawler.I_MaterialDefinition;

export namespace Brawler
{
	class MaterialDefinitionHandle
	{
	public:
		MaterialDefinitionHandle() = default;
		explicit MaterialDefinitionHandle(I_MaterialDefinition& materialDefinition);

		~MaterialDefinitionHandle();

		MaterialDefinitionHandle(const MaterialDefinitionHandle& rhs) = delete;
		MaterialDefinitionHandle& operator=(const MaterialDefinitionHandle& rhs) = delete;

		MaterialDefinitionHandle(MaterialDefinitionHandle&& rhs) noexcept;
		MaterialDefinitionHandle& operator=(MaterialDefinitionHandle&& rhs) noexcept;

		std::uint32_t GetGPUSceneBufferIndex() const;

	private:
		void NotifyMaterialDefinitionForDeletion();

	private:
		I_MaterialDefinition* mMaterialDefinitionPtr;
	};
}