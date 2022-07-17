module;

export module Brawler.ZSTDContext;
import :UnderlyingZSTDContextTypes;
import :WrappedZSTDContext;

export namespace Brawler
{
	using ZSTDCompressionContext = WrappedZSTDContext<ZSTDCompressionContextIMPL>;
	using ZSTDDecompressionContext = WrappedZSTDContext<ZSTDDecompressionContextIMPL>;
}
