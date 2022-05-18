module;

module Brawler.FormatConversionModelTextureUpdateState;
import Brawler.Application;
import Brawler.D3D12.Renderer;

namespace Brawler
{
	BC7CompressionRenderModule& GetBC7CompressionRenderModule()
	{
		return Brawler::GetRenderer().GetRenderModule<BC7CompressionRenderModule>();
	}
}