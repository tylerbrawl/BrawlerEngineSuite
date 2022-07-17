module;
#include <memory>
#include <cassert>
#include <zstd.h>

export module Brawler.ZSTDContext:UnderlyingZSTDContextTypes;

namespace Brawler
{
	// I was going to make these constexpr lambda functions, but it turns out that if you do that,
		// then you can't call std::unique_ptr::operator=() properly.

	struct CompressionContextDeleter
	{
		void operator()(ZSTD_CCtx* contextPtr) const
		{
			if (contextPtr != nullptr)
			{
				const std::size_t deleteResult = ZSTD_freeCCtx(contextPtr);
				assert(!ZSTD_isError(deleteResult) && "ERROR: ZSTD failed to delete a compression context (ZSTD_CCtx)!");
			}
		}
	};

	struct DecompressionContextDeleter
	{
		void operator()(ZSTD_DCtx* contextPtr) const
		{
			if (contextPtr != nullptr)
			{
				const std::size_t deleteResult = ZSTD_freeDCtx(contextPtr);
				assert(!ZSTD_isError(deleteResult) && "ERROR: ZSTD failed to delete a decompression context (ZSTD_DCtx)!");
			}
		}
	};
}

export namespace Brawler
{
	using ZSTDCompressionContextIMPL = std::unique_ptr<ZSTD_CCtx, CompressionContextDeleter>;
	using ZSTDDecompressionContextIMPL = std::unique_ptr<ZSTD_DCtx, DecompressionContextDeleter>;

	template <typename T>
	concept IsZSTDContextType = (std::is_same_v<T, ZSTDCompressionContextIMPL> || std::is_same_v<T, ZSTDDecompressionContextIMPL>);
}