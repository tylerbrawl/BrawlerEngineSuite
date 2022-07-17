module;
#include <span>
#include <vector>
#include <cassert>
#include <zstd.h>
#include "DxDef.h"

export module Brawler.ZSTDCompressionOperation;
import Brawler.ZSTDContext;
import Util.ZSTD;
import Util.General;

export namespace Brawler
{
	enum class ZSTDCompressionLevel
	{
		PREFER_SPEED = 1,
		PREFER_QUALITY = 19
	};
}

namespace Brawler
{
	namespace IMPL
	{
		template <ZSTDCompressionLevel CompressionLevel>
		class ZSTDCompressionOperation
		{
		public:
			struct CompressionResults
			{
				std::vector<std::byte> CompressedByteArr;
				HRESULT HResult;
			};

		public:
			ZSTDCompressionOperation() = default;

			ZSTDCompressionOperation(const ZSTDCompressionOperation& rhs) = delete;
			ZSTDCompressionOperation& operator=(const ZSTDCompressionOperation& rhs) = delete;

			ZSTDCompressionOperation(ZSTDCompressionOperation&& rhs) noexcept = default;
			ZSTDCompressionOperation& operator=(ZSTDCompressionOperation&& rhs) noexcept = default;

			HRESULT BeginCompressionOperation(const std::span<const std::byte> srcDataSpan);
			HRESULT ContinueCompressionOperation(const std::span<std::byte> destDataSpan);

			CompressionResults FinishCompressionOperation();
			HRESULT FinishCompressionOperation(const std::span<std::byte> destDataSpan);

			bool IsCompressionComplete() const;

			std::size_t GetZSTDBlockSize() const;

		private:
			ZSTDCompressionContext mCompressionContext;
			ZSTD_inBuffer mInputBuffer;
			std::span<const std::byte> mSrcDataSpan;
			bool mOperationCompleted;
		};
	}
}

// Explicit template instantiations are provided in ZSTDCompressionOperation.cpp.

export namespace Brawler
{
	using ZSTDCompressionOperation = IMPL::ZSTDCompressionOperation<ZSTDCompressionLevel::PREFER_QUALITY>;
	using FastZSTDCompressionOperation = IMPL::ZSTDCompressionOperation<ZSTDCompressionLevel::PREFER_SPEED>;
}