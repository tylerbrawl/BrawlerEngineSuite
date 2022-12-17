module;
#include <cassert>
#include <concepts>
#include <memory>
#include <DxDef.h>

export module Brawler.SceneTextures;
import Brawler.D3D12.I_GPUResource;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.BindlessSRVAllocation;
import Util.Engine;

namespace Brawler
{
	template <typename TextureType>
	class SceneTexture
	{
	public:
		SceneTexture() = default;
		explicit SceneTexture(std::unique_ptr<TextureType>&& texturePtr);

		SceneTexture(const SceneTexture& rhs) = delete;
		SceneTexture& operator=(const SceneTexture& rhs) = delete;

		SceneTexture(SceneTexture&& rhs) noexcept = default;
		SceneTexture& operator=(SceneTexture&& rhs) noexcept = default;

		void SetTexture(std::unique_ptr<TextureType>&& texturePtr);

		TextureType& GetTexture();
		const TextureType& GetTexture() const;

		std::uint32_t GetBindlessSRVIndex() const;

	private:
		static consteval D3D12_SRV_DIMENSION GetSRVDimension();

		void SetBindlessSRVAllocation(TextureType& texture);

	private:
		std::unique_ptr<TextureType> mTexturePtr;
		D3D12::BindlessSRVAllocation mBindlessSRVAllocation;
	};
}

// ----------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename TextureType>
	SceneTexture<TextureType>::SceneTexture(std::unique_ptr<TextureType>&& texturePtr) :
		mTexturePtr(std::move(texturePtr)),
		mBindlessSRVAllocation()
	{
		assert(mTexturePtr != nullptr);
		SetBindlessSRVAllocation(*mTexturePtr);
	}

	template <typename TextureType>
	void SceneTexture<TextureType>::SetTexture(std::unique_ptr<TextureType>&& texturePtr)
	{
		assert(texturePtr != nullptr);
		
		mBindlessSRVAllocation = texturePtr->CreateBindlessSRV();
		mTexturePtr = std::move(texturePtr);
	}

	template <typename TextureType>
	TextureType& SceneTexture<TextureType>::GetTexture()
	{
		assert(mTexturePtr != nullptr);
		return *mTexturePtr;
	}

	template <typename TextureType>
	const TextureType& SceneTexture<TextureType>::GetTexture() const
	{
		assert(mTexturePtr != nullptr);
		return *mTexturePtr;
	}

	template <typename TextureType>
	std::uint32_t SceneTexture<TextureType>::GetBindlessSRVIndex() const
	{
		assert(mTexturePtr != nullptr);
		return mBindlessSRVAllocation.GetBindlessSRVIndex();
	}

	template <typename TextureType>
	consteval D3D12_SRV_DIMENSION SceneTexture<TextureType>::GetSRVDimension()
	{
		if constexpr (std::is_same_v<TextureType, D3D12::Texture2D>)
			return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_TEXTURE2D;
		else
		{
			static_assert(sizeof(TextureType) != sizeof(TextureType), "ERROR: SceneTexture<TextureType>::GetSRVDimension() needs to be updated to account for an unrecognized TextureType!");
			return D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_UNKNOWN;
		}
	}

	template <typename TextureType>
	void SceneTexture<TextureType>::SetBindlessSRVAllocation(TextureType& texture)
	{
		static constexpr D3D12_SRV_DIMENSION SRV_DIMENSION = GetSRVDimension();

		D3D12::I_GPUResource& textureResource{ static_cast<D3D12::I_GPUResource&>(texture) };
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format = textureResource.GetResourceDescription().Format,
			.ViewDimension = SRV_DIMENSION,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};

		if constexpr (std::is_same_v<TextureType, D3D12::Texture2D>)
		{
			assert(D3D12GetFormatPlaneCount(&(Util::Engine::GetD3D12Device()), textureResource.GetResourceDescription().Format) == 1 && "Congratulations! You found a multi-planar texture format which isn't meant for depth/stencil textures. Have fun figuring out how to create descriptors for it~");

			srvDesc.Texture2D = D3D12_TEX2D_SRV{
				.MostDetailedMip = 0,
				.MipLevels = textureResource.GetResourceDescription().MipLevels,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			};
		}
		else
			static_assert(sizeof(TextureType) != sizeof(TextureType), "ERROR: SceneTexture<TextureType>::SetBindlessSRVAllocation() needs to be updated to account for an unrecognized TextureType!");

		mBindlessSRVAllocation = textureResource.CreateBindlessSRV(std::move(srvDesc));
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------

export namespace Brawler
{
	using SceneTexture2D = SceneTexture<D3D12::Texture2D>;
}