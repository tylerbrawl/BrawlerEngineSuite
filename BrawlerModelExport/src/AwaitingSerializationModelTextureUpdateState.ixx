module;
#include <optional>
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.AwaitingSerializationModelTextureUpdateState;
import Brawler.I_ModelTextureUpdateState;

export namespace Brawler
{
	template <aiTextureType TextureType>
	class AwaitingSerializationModelTextureUpdateState final : public I_ModelTextureUpdateState<AwaitingSerializationModelTextureUpdateState<TextureType>, TextureType>
	{
	public:
		AwaitingSerializationModelTextureUpdateState() = default;

		AwaitingSerializationModelTextureUpdateState(const AwaitingSerializationModelTextureUpdateState& rhs) = delete;
		AwaitingSerializationModelTextureUpdateState& operator=(const AwaitingSerializationModelTextureUpdateState& rhs) = delete;

		AwaitingSerializationModelTextureUpdateState(AwaitingSerializationModelTextureUpdateState&& rhs) noexcept = default;
		AwaitingSerializationModelTextureUpdateState& operator=(AwaitingSerializationModelTextureUpdateState&& rhs) noexcept = default;

		void UpdateTextureScratchImage(DirectX::ScratchImage& image);
		bool IsFinalTextureReadyForSerialization() const;

		std::optional<AwaitingSerializationModelTextureUpdateState> GetNextState() const;
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	void AwaitingSerializationModelTextureUpdateState<TextureType>::UpdateTextureScratchImage(DirectX::ScratchImage& image)
	{}

	template <aiTextureType TextureType>
	bool AwaitingSerializationModelTextureUpdateState<TextureType>::IsFinalTextureReadyForSerialization() const
	{
		// In this state, we know that the texture is ready to be serialized.

		return true;
	}

	template <aiTextureType TextureType>
	std::optional<AwaitingSerializationModelTextureUpdateState<TextureType>> AwaitingSerializationModelTextureUpdateState<TextureType>::GetNextState() const
	{
		return std::optional<AwaitingSerializationModelTextureUpdateState<TextureType>>{};
	}
}