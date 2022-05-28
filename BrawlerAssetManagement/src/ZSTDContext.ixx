module;

export module Brawler.AssetManagement.ZSTDContext;
import :UnderlyingZSTDContextTypes;
import :WrappedZSTDContext;

export namespace Brawler
{
	namespace AssetManagement
	{
		using ZSTDCompressionContext = WrappedZSTDContext<ZSTDCompressionContextIMPL>;
		using ZSTDDecompressionContext = WrappedZSTDContext<ZSTDDecompressionContextIMPL>;
	}
}
