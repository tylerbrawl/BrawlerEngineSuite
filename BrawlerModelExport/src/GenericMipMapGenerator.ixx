module;
#include <span>
#include <optional>
#include <cassert>
#include <DxDef.h>
#include <DirectXTex.h>

export module Brawler.MipMapGeneration:GenericMipMapGenerator;
import Brawler.D3D12.Texture2D;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.DescriptorTableBuilder;
import Brawler.D3D12.GPUCommandContexts;
import Brawler.D3D12.PipelineEnums;
import Brawler.D3D12.GPUResourceViews;

export namespace Brawler
{
	template <DXGI_FORMAT TextureFormat>
	class GenericMipMapGenerator
	{
	public:
		explicit GenericMipMapGenerator(D3D12::Texture2D& textureToMipMap, const std::size_t startingMipLevel = 0);

		GenericMipMapGenerator(const GenericMipMapGenerator& rhs) = delete;
		GenericMipMapGenerator& operator=(const GenericMipMapGenerator& rhs) = delete;

		GenericMipMapGenerator(GenericMipMapGenerator&& rhs) noexcept = default;
		GenericMipMapGenerator& operator=(GenericMipMapGenerator&& rhs) noexcept = default;

		void CreateMipMapGenerationRenderPasses(D3D12::FrameGraphBuilder& frameGraphBuilder) const;

	private:
		D3D12::Texture2D* mTexturePtr;
		std::size_t mStartingMipLevel;
	};
}

// -----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <DXGI_FORMAT TextureFormat>
	GenericMipMapGenerator<TextureFormat>::GenericMipMapGenerator(D3D12::Texture2D& textureToMipMap, const std::size_t startingMipLevel) :
		mTexturePtr(&textureToMipMap),
		mStartingMipLevel(startingMipLevel)
	{
		assert(mStartingMipLevel < mTexturePtr->GetResourceDescription().MipLevels && "ERROR: An out-of-bounds mip level index was specified when constructing a GenericMipMapGenerator instance!");
	}

	template <DXGI_FORMAT TextureFormat>
	void GenericMipMapGenerator<TextureFormat>::CreateMipMapGenerationRenderPasses(D3D12::FrameGraphBuilder& frameGraphBuilder) const
	{
		static constexpr std::size_t MAX_OUTPUT_TEXTURES_PER_PASS = 2;
		
		// Create a RenderPass for every mip level which we need to generate, starting from
		// mStartingMipLevel and ending with the texture's last mip level.
		const std::size_t numTotalMipLevels = mTexturePtr->GetResourceDescription().MipLevels;
		std::size_t numMipLevelsToGenerate = (numTotalMipLevels - mStartingMipLevel - 1);

		if (numMipLevelsToGenerate == 0) [[unlikely]]
			return;

		D3D12::RenderPassBundle mipMapGenerationBundle{};
		std::size_t currInputMipLevel = mStartingMipLevel;

		while (numMipLevelsToGenerate > 0)
		{
			struct MipMapGenerationInfo
			{
				D3D12::Texture2DSubResource InputTextureSubResource;
				D3D12::Texture2DSubResource OutputMip1SubResource;
				std::optional<D3D12::Texture2DSubResource> OutputMip2SubResource;
			};

			std::size_t mipLevelsGeneratedThisPass = 0;

			D3D12::RenderPass<GPUCommandQueueType::DIRECT, MipMapGenerationInfo> mipMapGenerationPass{};
			mipMapGenerationPass.SetRenderPassName("Mip Map Generation Pass");

			MipMapGenerationInfo generationInfo{};

			generationInfo.InputTextureSubResource = mTexturePtr->GetSubResource(currInputMipLevel);
			mipMapGenerationPass.AddResourceDependency(generationInfo.InputTextureSubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			assert((currInputMipLevel + 1) < numTotalMipLevels);
			generationInfo.OutputMip1SubResource = mTexturePtr->GetSubResource(currInputMipLevel + 1);
			mipMapGenerationPass.AddResourceDependency(generationInfo.OutputMip1SubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			++mipLevelsGeneratedThisPass;

			if ((currInputMipLevel + 2) < numTotalMipLevels)
			{
				generationInfo.OutputMip2SubResource = mTexturePtr->GetSubResource(currInputMipLevel + 2);
				mipMapGenerationPass.AddResourceDependency(generationInfo.OutputMip2SubResource, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				++mipLevelsGeneratedThisPass;
			}

			mipMapGenerationPass.SetInputData(std::move(generationInfo));

			mipMapGenerationPass.SetRenderPassCommands([] (D3D12::DirectContext& context, const MipMapGenerationInfo& generationInfo)
			{
				auto resourceBinder = context.SetPipelineState<Brawler::PSOs::PSOID::GENERIC_DOWNSAMPLE>();

				D3D12::DescriptorTableBuilder tableBuilder{ 3 };
				tableBuilder.CreateShaderResourceView(0, InputTextureSubResource.CreateShaderResourceView());
				tableBuilder.CreateUnorderedAccessView(1, OutputMip1SubResource.CreateUnorderedAccessView());
				
			})
		}
	}
}