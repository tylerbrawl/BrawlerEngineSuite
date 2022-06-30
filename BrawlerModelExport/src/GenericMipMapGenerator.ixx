module;
#include <span>
#include <optional>
#include <cassert>
#include <memory>
#include <DxDef.h>
#include <DirectXTex.h>

export module Brawler.MipMapGeneration:GenericMipMapGenerator;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.PipelineEnums;
import Brawler.D3D12.GPUResourceBinding;

namespace Brawler
{
	template <DXGI_FORMAT Format>
	consteval bool IsSRGB()
	{
		switch (Format)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: [[fallthrough]];
		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
			return true;

		default:
			return false;
		}
	}

	template <DXGI_FORMAT Format>
	consteval DXGI_FORMAT GetUAVFormatForSRGBFormat()
	{
		switch (Format)
		{
		case DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM;

		case DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB:
			return DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB;

		default:
			assert(false);

			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		}
	}

	template <DXGI_FORMAT Format>
	struct FormatInfo
	{
	private:
		static constexpr bool IS_SRGB = IsSRGB<Format>();

	public:
		static constexpr Brawler::PSOs::PSOID PSO_IDENTIFIER = (IS_SRGB ? Brawler::PSOs::PSOID::GENERIC_DOWNSAMPLE_SRGB : Brawler::PSOs::PSOID::GENERIC_DOWNSAMPLE);
		static constexpr DXGI_FORMAT UAV_FORMAT = (IS_SRGB ? GetUAVFormatForSRGBFormat<Format>() : Format);
	};
}

export namespace Brawler
{
	template <DXGI_FORMAT TextureFormat>
	class GenericMipMapGenerator
	{
	public:
		explicit GenericMipMapGenerator(D3D12::Texture2D& textureToMipMap, const std::uint32_t startingMipLevel = 0);

		GenericMipMapGenerator(const GenericMipMapGenerator& rhs) = delete;
		GenericMipMapGenerator& operator=(const GenericMipMapGenerator& rhs) = delete;

		GenericMipMapGenerator(GenericMipMapGenerator&& rhs) noexcept = default;
		GenericMipMapGenerator& operator=(GenericMipMapGenerator&& rhs) noexcept = default;

		void CreateMipMapGenerationRenderPasses(D3D12::FrameGraphBuilder& frameGraphBuilder) const;

	private:
		D3D12::Texture2D* mTexturePtr;
		std::uint32_t mStartingMipLevel;
	};
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	struct MipMapGenerationInfo
	{
		std::unique_ptr<D3D12::DescriptorTableBuilder> TableBuilderPtr;
		DirectX::XMUINT2 OutputDimensions;
		std::uint32_t StartingMipLevel;
		std::size_t NumMipLevelsGenerated;
	};

	struct MipMapConstants
	{
		DirectX::XMFLOAT2 InverseOutputDimensions;
		std::uint32_t StartingMipLevel;
		std::uint32_t OutputMipLevelCount;
	};
}

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat>
	GenericMipMapGenerator<TextureFormat>::GenericMipMapGenerator(D3D12::Texture2D& textureToMipMap, const std::uint32_t startingMipLevel) :
		mTexturePtr(&textureToMipMap),
		mStartingMipLevel(startingMipLevel)
	{
		assert(mStartingMipLevel < mTexturePtr->GetResourceDescription().MipLevels && "ERROR: An out-of-bounds mip level index was specified when constructing a GenericMipMapGenerator instance!");
	}

	template <DXGI_FORMAT TextureFormat>
	void GenericMipMapGenerator<TextureFormat>::CreateMipMapGenerationRenderPasses(D3D12::FrameGraphBuilder& frameGraphBuilder) const
	{
		static constexpr std::size_t MAX_OUTPUT_TEXTURES_PER_PASS = 2;

		const Brawler::D3D12_RESOURCE_DESC& textureDesc{ mTexturePtr->GetResourceDescription() };
		
		// Create a RenderPass for every mip level which we need to generate, starting from
		// mStartingMipLevel and ending with the texture's last mip level.
		const std::size_t numTotalMipLevels = textureDesc.MipLevels;
		std::size_t numMipLevelsToGenerate = (numTotalMipLevels - mStartingMipLevel - 1);

		const std::size_t originalWidth = textureDesc.Width;
		const std::size_t originalHeight = textureDesc.Height;

		if (numMipLevelsToGenerate == 0) [[unlikely]]
			return;

		D3D12::RenderPassBundle mipMapGenerationBundle{};
		std::uint32_t currInputMipLevel = mStartingMipLevel;

		while (numMipLevelsToGenerate > 0)
		{
			std::size_t mipLevelsGeneratedThisPass = 0;

			D3D12::RenderPass<D3D12::GPUCommandQueueType::DIRECT, MipMapGenerationInfo> mipMapGenerationPass{};
			mipMapGenerationPass.SetRenderPassName("Mip Map Generation Pass");

			MipMapGenerationInfo generationInfo{};

			generationInfo.TableBuilderPtr = std::make_unique<D3D12::DescriptorTableBuilder>(3);

			D3D12::Texture2DSubResource inputTextureSubResource{ mTexturePtr->GetSubResource(currInputMipLevel) };
			mipMapGenerationPass.AddResourceDependency(inputTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			generationInfo.TableBuilderPtr->CreateShaderResourceView(0, inputTextureSubResource.CreateShaderResourceView<TextureFormat>());

			assert((static_cast<std::size_t>(currInputMipLevel) + 1) < numTotalMipLevels);
			D3D12::Texture2DSubResource outputMip1SubResource{ mTexturePtr->GetSubResource(currInputMipLevel + 1) };
			mipMapGenerationPass.AddResourceDependency(outputMip1SubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			generationInfo.TableBuilderPtr->CreateUnorderedAccessView(1, outputMip1SubResource.CreateUnorderedAccessView<FormatInfo<TextureFormat>::UAV_FORMAT>());
			++mipLevelsGeneratedThisPass;

			if ((static_cast<std::size_t>(currInputMipLevel) + 2) < numTotalMipLevels)
			{
				D3D12::Texture2DSubResource outputMip2SubResource{ mTexturePtr->GetSubResource(currInputMipLevel + 2) };
				mipMapGenerationPass.AddResourceDependency(outputMip2SubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				generationInfo.TableBuilderPtr->CreateUnorderedAccessView(2, outputMip2SubResource.CreateUnorderedAccessView<FormatInfo<TextureFormat>::UAV_FORMAT>());
				++mipLevelsGeneratedThisPass;
			}
			else
				generationInfo.TableBuilderPtr->NullifyUnorderedAccessView<FormatInfo<TextureFormat>::UAV_FORMAT, D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D>(2);

			generationInfo.OutputDimensions.x = static_cast<std::uint32_t>(originalWidth / (static_cast<std::uint64_t>(1) << currInputMipLevel));
			generationInfo.OutputDimensions.y = static_cast<std::uint32_t>(originalHeight / (static_cast<std::uint64_t>(1) << currInputMipLevel));

			generationInfo.StartingMipLevel = currInputMipLevel;
			generationInfo.NumMipLevelsGenerated = mipLevelsGeneratedThisPass;

			mipMapGenerationPass.SetInputData(std::move(generationInfo));

			mipMapGenerationPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const MipMapGenerationInfo& generationInfo)
			{
				using RootParams = Brawler::RootParameters::GenericDownsample;

				auto resourceBinder = context.SetPipelineState<FormatInfo<TextureFormat>::PSO_IDENTIFIER>();
				resourceBinder.BindDescriptorTable<RootParams::TEXTURES_TABLE>(generationInfo.TableBuilderPtr->GetDescriptorTable());

				// Check for validity when setting these values as root constants.
				static_assert(sizeof(MipMapConstants) == 4 * sizeof(std::uint32_t));

				MipMapConstants constants{};
				DirectX::XMStoreFloat2(&(constants.InverseOutputDimensions), DirectX::XMVectorReciprocal(DirectX::XMLoadUInt2(&(generationInfo.OutputDimensions))));
				constants.StartingMipLevel = generationInfo.StartingMipLevel;
				constants.OutputMipLevelCount = static_cast<std::uint32_t>(generationInfo.NumMipLevelsGenerated);

				resourceBinder.BindRoot32BitConstants<RootParams::MIP_MAP_CONSTANTS>(constants);

				static constexpr std::uint32_t NUM_X_THREADS_PER_GROUP = 8;
				static constexpr std::uint32_t NUM_Y_THREADS_PER_GROUP = 8;

				const std::uint32_t numXThreadGroups = (generationInfo.OutputDimensions.x % 8 == 0 ? (generationInfo.OutputDimensions.x / 8) : (generationInfo.OutputDimensions.x / 8) + 1);
				const std::uint32_t numYThreadGroups = (generationInfo.OutputDimensions.y % 8 == 0 ? (generationInfo.OutputDimensions.y / 8) : (generationInfo.OutputDimensions.y / 8) + 1);

				context.Dispatch2D(numXThreadGroups, numYThreadGroups);
			});

			mipMapGenerationBundle.AddRenderPass(std::move(mipMapGenerationPass));

			numMipLevelsToGenerate -= mipLevelsGeneratedThisPass;

			// Even if we only output one mip level, we'll be exiting the loop anyways, so
			// we can just always add 2 to currInputMipLevel.
			currInputMipLevel += MAX_OUTPUT_TEXTURES_PER_PASS;
		}

		frameGraphBuilder.AddRenderPassBundle(std::move(mipMapGenerationBundle));
	}
}