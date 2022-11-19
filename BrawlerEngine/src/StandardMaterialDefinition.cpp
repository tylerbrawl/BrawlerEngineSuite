module;
#include <cassert>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

module Brawler.StandardMaterialDefinition;
import Brawler.GPUSceneManager;
import Brawler.GPUSceneBufferID;
import Brawler.D3D12.BufferResource;
import Brawler.D3D12.Renderer;
import Brawler.GPUSceneUpdateRenderModule;
import Brawler.GPUSceneBufferUpdateOperation;
import Brawler.Application;
import Brawler.SceneTextureDatabase;

namespace Brawler
{
	StandardMaterialDefinition::StandardMaterialDefinition(const StandardMaterialBuilder& builder) :
		mMaterialDescriptorSubAllocation(),
		mDependentTextureHashArr(),
		mHandleCollection()
	{
		[[maybe_unused]] const bool wasReservationSuccessful = GPUSceneManager::GetInstance().GetGPUSceneBufferResource<GPUSceneBufferID::MATERIAL_DESCRIPTOR_BUFFER>().AssignReservation(mMaterialDescriptorSubAllocation);
		assert(wasReservationSuccessful && "ERROR: The maximum number of GPUSceneBuffer MaterialDescriptor instances has been exhausted!");

		InitializeSceneTextureHandles(builder);
	}

	std::span<const FilePathHash> StandardMaterialDefinition::GetDependentSceneTextureFilePathHashSpan() const
	{
		return std::span<const FilePathHash>{ mDependentTextureHashArr };
	}

	void StandardMaterialDefinition::UpdateGPUSceneMaterialDescriptor() const
	{
		static constexpr std::uint32_t UNUSED_TEXTURE_SRV_INDEX = std::numeric_limits<std::uint32_t>::max();
		
		const GPUSceneTypes::MaterialDescriptor materialDescriptor{
			.BaseColorTextureSRVIndex = (mHandleCollection.HBaseColorTexture.has_value() ? mHandleCollection.HBaseColorTexture->GetBindlessSRVIndex() : UNUSED_TEXTURE_SRV_INDEX),
			.NormalMapSRVIndex = (mHandleCollection.HNormalMap.has_value() ? mHandleCollection.HNormalMap->GetBindlessSRVIndex() : UNUSED_TEXTURE_SRV_INDEX),
			.RoughnessTextureSRVIndex = (mHandleCollection.HRoughnessTexture.has_value() ? mHandleCollection.HRoughnessTexture->GetBindlessSRVIndex() : UNUSED_TEXTURE_SRV_INDEX),
			.MetallicTextureSRVIndex = (mHandleCollection.HMetallicTexture.has_value() ? mHandleCollection.HMetallicTexture->GetBindessSRVIndex() : UNUSED_TEXTURE_SRV_INDEX)
		};

		GPUSceneBufferUpdateOperation<GPUSceneBufferID::MATERIAL_DESCRIPTOR_BUFFER> descriptorUpdateOperation{ mMaterialDescriptorSubAllocation.GetBufferCopyRegion() };
		descriptorUpdateOperation.SetUpdateSourceData(std::span<const GPUSceneTypes::MaterialDescriptor>{ &materialDescriptor, 1 });

		Brawler::GetRenderer().GetRenderModule<GPUSceneUpdateRenderModule>().ScheduleGPUSceneBufferUpdateForNextFrame(std::move(descriptorUpdateOperation));
	}

	std::uint32_t StandardMaterialDefinition::GetGPUSceneBufferIndex() const
	{
		return (mMaterialDescriptorSubAllocation.GetOffsetFromBufferStart() / sizeof(GPUSceneTypes::MaterialDescriptor));
	}

	void StandardMaterialDefinition::InitializeSceneTextureHandles(const StandardMaterialBuilder& builder)
	{
		static constexpr auto INITIALIZE_SCENE_TEXTURE_HANDLE_LAMBDA = [] (std::optional<SceneTexture2DHandle>& hSceneTexture, const std::optional<FilePathHash> textureFilePathHash)
		{
			if (textureFilePathHash.has_value()) [[likely]]
			{
				mDependentTextureHashArr.push_back(*textureFilePathHash);

				hSceneTexture = Brawler::SceneTextureDatabase::GetInstance().CreateSceneTextureHandle<SceneTexture2DHandle>(*textureFilePathHash);
				assert(hSceneTexture.has_value() && "ERROR: A StandardMaterialDefinition instance was created before all of its required textures could be properly initialized!");
			}
		};
		
		INITIALIZE_SCENE_TEXTURE_HANDLE_LAMBDA(mHandleCollection.HBaseColorTexture, builder.GetBaseColorFilePathHash());
		INITIALIZE_SCENE_TEXTURE_HANDLE_LAMBDA(mHandleCollection.HNormalMap, builder.GetNormalMapFilePathHash());
		INITIALIZE_SCENE_TEXTURE_HANDLE_LAMBDA(mHandleCollection.HRoughnessTexture, builder.GetRoughnessFilePathHash());
		INITIALIZE_SCENE_TEXTURE_HANDLE_LAMBDA(mHandleCollection.HMetallicTexture, builder.GetMetallicFilePathHash());
	}
}