module;
#include <optional>
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.FormatConversionModelTextureUpdateState;
import Brawler.I_ModelTextureUpdateState;
import Brawler.AwaitingSerializationModelTextureUpdateState;

namespace Brawler
{
	template <aiTextureType TextureType>
	class CPUFormatConverter
	{
	public:
		CPUFormatConverter() = default;

		CPUFormatConverter(const CPUFormatConverter& rhs) = delete;
		CPUFormatConverter& operator=(const CPUFormatConverter& rhs) = delete;

		CPUFormatConverter(CPUFormatConverter&& rhs) noexcept = default;
		CPUFormatConverter& operator=(CPUFormatConverter&& rhs) noexcept = default;

		void UpdateTextureScratchImage(DirectX::ScratchImage& image);
		bool IsFinalTextureReadyForSerialization() const;
	};

	template <aiTextureType TextureType>
	class GPUBC7FormatConverter
	{
	public:
		GPUBC7FormatConverter() = default;

		GPUBC7FormatConverter(const GPUBC7FormatConverter& rhs) = delete;
		GPUBC7FormatConverter& operator=(const GPUBC7FormatConverter& rhs) = delete;

		GPUBC7FormatConverter(GPUBC7FormatConverter&& rhs) noexcept = default;
		GPUBC7FormatConverter& operator=(GPUBC7FormatConverter&& rhs) noexcept = default;

		void UpdateTextureScratchImage(DirectX::ScratchImage& image);
		bool IsFinalTextureReadyForSerialization() const;

	private:

	};
}

export namespace Brawler
{
	template <aiTextureType TextureType>
	class FormatConversionModelTextureUpdateState final : public I_ModelTextureUpdateState<FormatConversionModelTextureUpdateState>
	{
	public:
		FormatConversionModelTextureUpdateState() = default;

		FormatConversionModelTextureUpdateState(const FormatConversionModelTextureUpdateState& rhs) = delete;
		FormatConversionModelTextureUpdateState& operator=(const FormatConversionModelTextureUpdateState& rhs) = delete;

		FormatConversionModelTextureUpdateState(FormatConversionModelTextureUpdateState&& rhs) noexcept = default;
		FormatConversionModelTextureUpdateState& operator=(FormatConversionModelTextureUpdateState&& rhs) noexcept = default;

		void UpdateTextureScratchImage(DirectX::ScratchImage& image);
		bool IsFinalTextureReadyForSerialization() const;

		std::optional<AwaitingSerializationModelTextureUpdateState> GetNextState() const;
	};
}