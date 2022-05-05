module;
#include <span>
#include <zstd.h>

export module Brawler.ZSTDContext;
import Brawler.ZSTDFrame;

export namespace Brawler
{
	class ZSTDContext
	{
	public:
		ZSTDContext();
		~ZSTDContext();

		ZSTDContext(const ZSTDContext& rhs) = delete;
		ZSTDContext& operator=(const ZSTDContext& rhs) = delete;

		ZSTDContext(ZSTDContext&& rhs) noexcept;
		ZSTDContext& operator=(ZSTDContext&& rhs) noexcept;

		ZSTDFrame CompressData(const std::span<std::uint8_t> byteArr) const;

	private:
		void DeleteCompressionContext();

	private:
		ZSTD_CCtx* mCompressionContextPtr;
	};
}