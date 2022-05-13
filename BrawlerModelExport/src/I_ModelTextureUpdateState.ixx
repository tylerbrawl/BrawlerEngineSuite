module;
#include <optional>
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.I_ModelTextureUpdateState;

export namespace Brawler
{
	template <typename DerivedType, auto TextureType>
	class I_ModelTextureUpdateState
	{
	protected:
		I_ModelTextureUpdateState() = default;

	public:
		virtual ~I_ModelTextureUpdateState() = default;

		I_ModelTextureUpdateState(const I_ModelTextureUpdateState& rhs) = delete;
		I_ModelTextureUpdateState& operator=(const I_ModelTextureUpdateState& rhs) = delete;

		I_ModelTextureUpdateState(I_ModelTextureUpdateState&& rhs) noexcept = default;
		I_ModelTextureUpdateState& operator=(I_ModelTextureUpdateState&& rhs) noexcept = default;

		void UpdateTextureScratchImage(DirectX::ScratchImage& image);
		bool IsFinalTextureReadyForSerialization() const;

		auto GetNextState() const;
	};
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename DerivedType, auto TextureType>
	void I_ModelTextureUpdateState<DerivedType, TextureType>::UpdateTextureScratchImage(DirectX::ScratchImage& image)
	{
		static_cast<DerivedType*>(this)->UpdateTextureScratchImage(image);
	}

	template <typename DerivedType, auto TextureType>
	bool I_ModelTextureUpdateState<DerivedType, TextureType>::IsFinalTextureReadyForSerialization() const
	{
		return static_cast<const DerivedType*>(this)->IsFinalTextureReadyForSerialization();
	}

	template <typename DerivedType, auto TextureType>
	auto I_ModelTextureUpdateState<DerivedType, TextureType>::GetNextState() const
	{
		return static_cast<DerivedType*>(this)->GetNextState();
	}
}