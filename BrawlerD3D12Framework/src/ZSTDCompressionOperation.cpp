module;
#include <span>
#include <vector>
#include <cassert>
#include <zstd.h>
#include "DxDef.h"

module Brawler.ZSTDCompressionOperation;

namespace
{
	template <Brawler::ZSTDCompressionLevel CompressionLevel>
	bool IsCompressionLevelValid()
	{
		static const bool levelValidity = [] ()
		{
			const ZSTD_bounds compressionLevelBounds{ ZSTD_cParam_getBounds(ZSTD_cParameter::ZSTD_c_compressionLevel) };

			if (ZSTD_isError(compressionLevelBounds.error)) [[unlikely]]
				return false;

			// Valid compression level values are in the range [compressionLevelBounds.lowerBound, 
			// compressionLevelBounds.upperBound].
			return ((compressionLevelBounds.lowerBound <= std::to_underlying(CompressionLevel)) && (std::to_underlying(CompressionLevel) <= compressionLevelBounds.upperBound));
		}();

		return levelValidity;
	}
}

namespace Brawler
{
	namespace IMPL
	{
		template <ZSTDCompressionLevel CompressionLevel>
		HRESULT ZSTDCompressionOperation<CompressionLevel>::BeginCompressionOperation(const std::span<const std::byte> srcDataSpan)
		{
			if (srcDataSpan.empty()) [[unlikely]]
				return E_INVALIDARG;

			std::size_t zstdError = ZSTD_CCtx_reset(mCompressionContext.Get(), ZSTD_ResetDirective::ZSTD_reset_session_and_parameters);

			if (ZSTD_isError(zstdError)) [[unlikely]]
				return Util::ZSTD::ZSTDErrorToHRESULT(zstdError);

			if (IsCompressionLevelValid<CompressionLevel>()) [[likely]]
			{
				zstdError = ZSTD_CCtx_setParameter(mCompressionContext.Get(), ZSTD_cParameter::ZSTD_c_compressionLevel, std::to_underlying(CompressionLevel));

				// If that failed, then revert back to the default parameters.
				if (ZSTD_isError(zstdError)) [[unlikely]]
				{
					zstdError = ZSTD_CCtx_reset(mCompressionContext.Get(), ZSTD_ResetDirective::ZSTD_reset_parameters);

					// If THAT failed, then abort.
					if (ZSTD_isError(zstdError)) [[unlikely]]
						return Util::ZSTD::ZSTDErrorToHRESULT(zstdError);
				}
			}

				mInputBuffer = ZSTD_inBuffer{
					.src = srcDataSpan.data(),
					.size = srcDataSpan.size_bytes(),
					.pos = 0
				};
				mSrcDataSpan = srcDataSpan;
				mOperationCompleted = false;

				return S_OK;
		}

		template <ZSTDCompressionLevel CompressionLevel>
		HRESULT ZSTDCompressionOperation<CompressionLevel>::ContinueCompressionOperation(const std::span<std::byte> destDataSpan)
		{
			if (destDataSpan.empty() || IsCompressionComplete()) [[unlikely]]
				return S_OK;

			ZSTD_outBuffer outputBuffer{
				.dst = destDataSpan.data(),
				.size = destDataSpan.size_bytes(),
				.pos = 0
			};

			const std::size_t zstdError = ZSTD_compressStream2(
				mCompressionContext.Get(),
				&outputBuffer,
				&mInputBuffer,
				ZSTD_EndDirective::ZSTD_e_flush
			);

			if (mInputBuffer.pos == mInputBuffer.size)
				mOperationCompleted = true;

			if (ZSTD_isError(zstdError)) [[unlikely]]
				return Util::ZSTD::ZSTDErrorToHRESULT(zstdError);

			return S_OK;
		}

		template <ZSTDCompressionLevel CompressionLevel>
		ZSTDCompressionOperation<CompressionLevel>::CompressionResults ZSTDCompressionOperation<CompressionLevel>::FinishCompressionOperation()
		{
			if (IsCompressionComplete()) [[unlikely]]
			{
				return CompressionResults{
					.CompressedByteArr{},
					.HResult = S_OK
				};
			}

			using Block = std::vector<std::byte>;

			std::vector<Block> blockArr{};
			std::size_t numBytesCompressed = 0;

			while (!IsCompressionComplete())
			{
				Block currBlock{};
				currBlock.resize(GetZSTDBlockSize());

				ZSTD_outBuffer outputBuffer{
					.dst = currBlock.data(),
					.size = currBlock.size(),
					.pos = 0
				};

				assert(outputBuffer.size > 0);

				const std::size_t zstdError = ZSTD_compressStream2(
					mCompressionContext.Get(),
					&outputBuffer,
					&mInputBuffer,
					ZSTD_EndDirective::ZSTD_e_flush
				);

				if (ZSTD_isError(zstdError)) [[unlikely]]
				{
					return CompressionResults{
						.CompressedByteArr{},
						.HResult = Util::ZSTD::ZSTDErrorToHRESULT(zstdError)
					};
				}

				numBytesCompressed += outputBuffer.pos;

				if (mInputBuffer.pos == mInputBuffer.size)
				{
					if (outputBuffer.pos != outputBuffer.size) [[likely]]
					{
						currBlock.resize(outputBuffer.pos);
						currBlock.shrink_to_fit();
					}

					mOperationCompleted = true;
				}

				blockArr.push_back(std::move(currBlock));
			}

			// Move all of the compressed bytes into a single contiguous block of
			// memory.
			std::vector<std::byte> compressedByteArr{};
			compressedByteArr.resize(numBytesCompressed);

			std::span<std::byte> copyDestSpan{ compressedByteArr };

			for (const auto& block : blockArr)
			{
				std::memcpy(copyDestSpan.data(), block.data(), block.size());
				copyDestSpan = copyDestSpan.subspan(block.size());
			}

			return CompressionResults{
				.CompressedByteArr{ std::move(compressedByteArr) },
				.HResult = S_OK
			};
		}

		template <ZSTDCompressionLevel CompressionLevel>
		HRESULT ZSTDCompressionOperation<CompressionLevel>::FinishCompressionOperation(const std::span<std::byte> destDataSpan)
		{
			const HRESULT hr = ContinueCompressionOperation(destDataSpan);

			if (FAILED(hr)) [[unlikely]]
				return hr;

			return (IsCompressionComplete() ? S_OK : E_NOT_SUFFICIENT_BUFFER);
		}

		template <ZSTDCompressionLevel CompressionLevel>
		bool ZSTDCompressionOperation<CompressionLevel>::IsCompressionComplete() const
		{
			return mOperationCompleted;
		}

		template <ZSTDCompressionLevel CompressionLevel>
		std::size_t ZSTDCompressionOperation<CompressionLevel>::GetZSTDBlockSize() const
		{
			return ZSTD_CStreamOutSize();
		}
	}
}

namespace Brawler
{
	namespace IMPL
	{
		template class ZSTDCompressionOperation<ZSTDCompressionLevel::PREFER_QUALITY>;
		template class ZSTDCompressionOperation<ZSTDCompressionLevel::PREFER_SPEED>;
	}
}