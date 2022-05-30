module;
#include <assimp/scene.h>
#include <DxDef.h>
#include <DirectXTex.h>

export module Brawler.BCTextureCompressionContext;
import Brawler.TextureTypeMap;

export namespace Brawler
{
	template <aiTextureType TextureType>
	class BCTextureCompressionContext
	{
	public:
		explicit BCTextureCompressionContext(const DirectX::Image& srcImage);

		BCTextureCompressionContext(const BCTextureCompressionContext& rhs) = delete;
		BCTextureCompressionContext& operator=(const BCTextureCompressionContext& rhs) = delete;

		BCTextureCompressionContext(BCTextureCompressionContext&& rhs) noexcept = default;
		BCTextureCompressionContext& operator=(BCTextureCompressionContext&& rhs) noexcept = default;

		DirectX::Image GetCompressedTexture() const;
	
	private:
		const DirectX::Image* mSrcImage;
	};
}

// ----------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <aiTextureType TextureType>
	BCTextureCompressionContext<TextureType>::BCTextureCompressionContext(const DirectX::Image& srcImage) :
		mSrcImage(&srcImage)
	{}

	template <aiTextureType TextureType>
	DirectX::Image BCTextureCompressionContext<TextureType>::GetCompressedTexture() const
	{
		DirectX::ScratchImage destImage{};

		DirectX::TexMetadata destMetadata{ mSrcImage->GetMetadata() };
		destMetadata.format = Brawler::GetDesiredTextureFormat<TextureType>();
		CheckHRESULT(destImage.Initialize(destMetadata));


	}
}