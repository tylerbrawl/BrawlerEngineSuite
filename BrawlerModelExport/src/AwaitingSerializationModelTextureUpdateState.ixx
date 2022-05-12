module;
#include <optional>
#include <DirectXTex.h>

export module Brawler.AwaitingSerializationModelTextureUpdateState;
import Brawler.I_ModelTextureUpdateState;

export namespace Brawler
{
	class AwaitingSerializationModelTextureUpdateState final : public I_ModelTextureUpdateState<AwaitingSerializationModelTextureUpdateState>
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