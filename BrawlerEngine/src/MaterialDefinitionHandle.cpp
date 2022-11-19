module;
#include <cstdint>

module Brawler.MaterialDefinitionHandle;

namespace Brawler
{
	MaterialDefinitionHandle::MaterialDefinitionHandle(I_MaterialDefinition& materialDefinition) :
		mMaterialDefinitionPtr(&materialDefinition)
	{}

	MaterialDefinitionHandle::~MaterialDefinitionHandle()
	{
		NofifyMaterialDefinitionForDeletion();
	}

	MaterialDefinitionHandle::MaterialDefinitionHandle(MaterialDefinitionHandle&& rhs) noexcept :
		mMaterialDefinitionPtr(rhs.mMaterialDefinitionPtr)
	{
		rhs.mMaterialDefinitionPtr = nullptr;
	}

	MaterialDefinitionHandle& MaterialDefinitionHandle::operator=(MaterialDefinitionHandle&& rhs) noexcept
	{
		NotifyMaterialDefinitionForDeletion();

		mMaterialDefinitionPtr = rhs.mMaterialDefinitionPtr;
		rhs.mMaterialDefinitionPtr = nullptr;

		return *this;
	}

	std::uint32_t MaterialDefinitionHandle::GetGPUSceneBufferIndex() const
	{
		assert(mMaterialDefinitionPtr != nullptr);
		return mMaterialDefinitionPtr->GetGPUSceneBufferIndex();
	}

	void MaterialDefinitionHandle::NotifyMaterialDefinitionForDeletion()
	{
		if (mMaterialDefinitionPtr != nullptr) [[likely]]
		{
			mMaterialDefinitionPtr->NotifyForDeletion();

			mMaterialDefinitionPtr = nullptr;
		}
	}
}