module;
#include <optional>
#include <span>
#include <cassert>
#include <assimp/material.h>
#include <DirectXTex.h>

export module Brawler.FormatConversionModelTextureUpdateState;
import Brawler.I_ModelTextureUpdateState;
import Brawler.AwaitingSerializationModelTextureUpdateState;
import Brawler.BC7CompressionEventHandle;
import Brawler.TextureTypeMap;
import Util.General;
import Brawler.BC7CompressionRenderModule;

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
		bool IsFormatConversionFinished() const;
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
		bool IsFormatConversionFinished() const;

	private:
		void InitializeBC7ImageCompression(const DirectX::ScratchImage& image);
		void CheckForImageCompressionCompletion(DirectX::ScratchImage& image);

	private:
		DirectX::ScratchImage mDestScratchImage;
		std::vector<BC7CompressionEventHandle> mCompressionEventHandleArr;
		bool mCompressionCompleted;
	};

	template <aiTextureType TextureType>
	struct FormatConverterTypeSolver
	{
		static_assert(sizeof(TextureType) != sizeof(TextureType));
	};

	template <aiTextureType TextureType>
	concept NonCompressedTextureType = !Brawler::IsBlockCompressedFormat<Brawler::GetDesiredTextureFormat<TextureType>()>();

	template <aiTextureType TextureType>
		requires NonCompressedTextureType<TextureType>
	struct FormatConverterTypeSolver<TextureType>
	{
		using ConverterType = CPUFormatConverter<TextureType>;
	};

	template <aiTextureType TextureType>
	consteval bool IsDesiredFormatBC7Compressed()
	{
		constexpr DXGI_FORMAT DESIRED_FORMAT = Brawler::GetDesiredTextureFormat<TextureType>();

		switch (DESIRED_FORMAT)
		{
		case DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
			return true;

		default:
			return false;
		}
	}

	template <aiTextureType TextureType>
	concept BC7CompressedTextureType = IsDesiredFormatBC7Compressed<TextureType>();

	template <aiTextureType TextureType>
		requires BC7CompressedTextureType<TextureType>
	struct FormatConverterTypeSolver<TextureType>
	{
		using ConverterType = GPUBC7FormatConverter<TextureType>;
	};

	template <aiTextureType TextureType>
	using FormatConverterType = typename FormatConverterTypeSolver<TextureType>::ConverterType;
}

export namespace Brawler
{
	template <aiTextureType TextureType>
	class FormatConversionModelTextureUpdateState final : public I_ModelTextureUpdateState<FormatConversionModelTextureUpdateState<TextureType>, TextureType>, private FormatConverterType<TextureType>
	{
	public:
		FormatConversionModelTextureUpdateState() = default;

		FormatConversionModelTextureUpdateState(const FormatConversionModelTextureUpdateState& rhs) = delete;
		FormatConversionModelTextureUpdateState& operator=(const FormatConversionModelTextureUpdateState& rhs) = delete;

		FormatConversionModelTextureUpdateState(FormatConversionModelTextureUpdateState&& rhs) noexcept = default;
		FormatConversionModelTextureUpdateState& operator=(FormatConversionModelTextureUpdateState&& rhs) noexcept = default;

		void UpdateTextureScratchImage(DirectX::ScratchImage& image);
		bool IsFinalTextureReadyForSerialization() const;

		std::optional<AwaitingSerializationModelTextureUpdateState<TextureType>> GetNextState() const;
	};
}

// ------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	BC7CompressionRenderModule& GetBC7CompressionRenderModule();
}

namespace Brawler
{
	template <aiTextureType TextureType>
	void CPUFormatConverter<TextureType>::UpdateTextureScratchImage(DirectX::ScratchImage& image)
	{
		// Don't do any conversions if the intermediate and desired formats are the same.
		if constexpr (Brawler::GetIntermediateTextureFormat<TextureType>() == Brawler::GetDesiredTextureFormat<TextureType>())
			return;

		DirectX::ScratchImage finalImage{};

		Util::General::CheckHRESULT(DirectX::Convert(
			image.GetImages(),
			image.GetImageCount(),
			image.GetMetadata(),
			Brawler::GetDesiredTextureFormat<TextureType>(),
			DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT,
			DirectX::TEX_THRESHOLD_DEFAULT,
			finalImage
		));

		image = std::move(finalImage);
	}

	template <aiTextureType TextureType>
	bool CPUFormatConverter<TextureType>::IsFormatConversionFinished() const
	{
		// All of the work is completed immediately on the CPU in CPUFormatConverter::UpdateTextureScratchImage(),
		// so we always return true.

		return true;
	}
}

namespace Brawler
{
	template <aiTextureType TextureType>
	void GPUBC7FormatConverter<TextureType>::UpdateTextureScratchImage(DirectX::ScratchImage& image)
	{
		// Don't do any updates if the compression was already completed.
		if (mCompressionCompleted) [[unlikely]]
			return;
		
		// The first time this function is called, we initialize the destination
		// DirectX::ScratchImage.
		if (mDestScratchImage.GetImageCount() == 0) [[unlikely]]
			InitializeBC7ImageCompression(image);
		else
			CheckForImageCompressionCompletion(image);
	}

	template <aiTextureType TextureType>
	bool GPUBC7FormatConverter<TextureType>::IsFormatConversionFinished() const
	{
		return mCompressionCompleted;
	}

	template <aiTextureType TextureType>
	void GPUBC7FormatConverter<TextureType>::InitializeBC7ImageCompression(const DirectX::ScratchImage& image)
	{
		DirectX::TexMetadata srcImageMetadata{ image.GetMetadata() };
		srcImageMetadata.format = Brawler::GetDesiredTextureFormat<TextureType>();

		Util::General::CheckHRESULT(mDestScratchImage.Initialize(srcImageMetadata));

		// Register each DirectX::Image instance within the destination DirectX::ScratchImage with
		// the BC7ImageCompressionRenderModule.

		const std::span<const DirectX::Image> destImageSpan{ mDestScratchImage.GetImages(), mDestScratchImage.GetImageCount() };
		const std::span<const DirectX::Image> srcImageSpan{ image.GetImages(), image.GetImageCount() };

		std::size_t currIndex = 0;
		assert(srcImageSpan.size() == destImageSpan.size());

		for (std::size_t i = 0; i < destImageSpan.size(); ++i)
		{
			BC7CompressionRenderModule& compressionModule{ Brawler::GetBC7CompressionRenderModule() };
			BC7CompressionEventHandle hCompressionEvent{ compressionModule.MakeGPUCompressionRequest<TextureType>(srcImageSpan[i]) };

			hCompressionEvent.SetDestinationImage(destImageSpan[i]);

			mCompressionEventHandleArr.push_back(std::move(hCompressionEvent));
		}
	}

	template <aiTextureType TextureType>
	void GPUBC7FormatConverter<TextureType>::CheckForImageCompressionCompletion(DirectX::ScratchImage& image)
	{
		// Check each BC7CompressionEventHandle to see if the compressed image could be copied to its
		// assigned DirectX::Image instance within mDestScratchImage. If all of them have finished, then
		// we std::move mDestScratchImage into image and mark the compression as completed.

		for (const auto& hCompressionEvent : mCompressionEventHandleArr)
		{
			if (!hCompressionEvent.TryCopyCompressedImage()) [[likely]]
				return;
		}

		image = std::move(mDestScratchImage);
		mCompressionCompleted = true;
	}
}

namespace Brawler
{
	template <aiTextureType TextureType>
	void FormatConversionModelTextureUpdateState<TextureType>::UpdateTextureScratchImage(DirectX::ScratchImage& image)
	{
		FormatConverterType<TextureType>::UpdateTextureScratchImage(image);
	}

	template <aiTextureType TextureType>
	bool FormatConversionModelTextureUpdateState<TextureType>::IsFinalTextureReadyForSerialization() const
	{
		return false;
	}

	template <aiTextureType TextureType>
	std::optional<AwaitingSerializationModelTextureUpdateState<TextureType>> FormatConversionModelTextureUpdateState<TextureType>::GetNextState() const
	{
		return (FormatConverterType<TextureType>::IsFormatConversionFinished() ? AwaitingSerializationModelTextureUpdateState<TextureType>{} : std::optional<AwaitingSerializationModelTextureUpdateState<TextureType>>{});
	}
}