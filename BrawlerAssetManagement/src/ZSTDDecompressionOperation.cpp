module;
#include <span>
#include <vector>
#include <cassert>
#include <zstd.h>
#include <DxDef.h>

module Brawler.AssetManagement.ZSTDDecompressionOperation;
import Util.ZSTD;

namespace Brawler
{
	namespace AssetManagement
	{
		HRESULT ZSTDDecompressionOperation::BeginDecompressionOperation(std::span<const std::byte> srcDataSpan)
		{
			if (srcDataSpan.empty()) [[unlikely]]
				return E_INVALIDARG;

			// Reset the decompression context.
			std::size_t zstdError = ZSTD_DCtx_reset(mDecompressionContext.Get(), ZSTD_ResetDirective::ZSTD_reset_session_only);

			if (ZSTD_isError(zstdError)) [[unlikely]]
				return Util::ZSTD::ZSTDErrorToHRESULT(zstdError);

			zstdError = ZSTD_DCtx_refDDict(mDecompressionContext.Get(), nullptr);

			if (ZSTD_isError(zstdError)) [[unlikely]]
				return Util::ZSTD::ZSTDErrorToHRESULT(zstdError);

			mSrcDataSpan = std::move(srcDataSpan);

			mInputBuffer = ZSTD_inBuffer{
				.src = mSrcDataSpan.data(),
				.size = mSrcDataSpan.size_bytes(),
				.pos = 0
			};

			mOperationFinished = false;

			return S_OK;
		}

		HRESULT ZSTDDecompressionOperation::ContinueDecompressionOperation(const std::span<std::byte> destDataSpan)
		{
			if (destDataSpan.empty() || IsDecompressionComplete()) [[unlikely]]
				return S_OK;

			ZSTD_outBuffer outputBuffer{
				.dst = destDataSpan.data(),
				.size = destDataSpan.size_bytes(),
				.pos = 0
			};

			const std::size_t zstdError = ZSTD_decompressStream(mDecompressionContext.Get(), &outputBuffer, &mInputBuffer);

			if (ZSTD_isError(zstdError)) [[unlikely]]
				return Util::ZSTD::ZSTDErrorToHRESULT(zstdError);

			if (zstdError == 0)
				mOperationFinished = true;

			return S_OK;
		}

		ZSTDDecompressionOperation::DecompressionResults ZSTDDecompressionOperation::FinishDecompressionOperation()
		{
			if (IsDecompressionComplete()) [[unlikely]]
				return DecompressionResults{
					.DecompressedByteArr{},
					.HResult = S_OK
				};

			using ZSTDBlock = std::vector<std::byte>;

			std::vector<ZSTDBlock> dataBlockArr{};
			std::size_t numBytesDecompressed = 0;

			while (!IsDecompressionComplete())
			{
				// Create a new output buffer for a sub-set of the data which needs to be decompressed.
				ZSTDBlock currBlock{};
				currBlock.resize(GetZSTDBlockSize());

				ZSTD_outBuffer outputBuffer{
					.dst = currBlock.data(),
					.size = currBlock.size(),
					.pos = 0
				};

				const std::size_t zstdError = ZSTD_decompressStream(mDecompressionContext.Get(), &outputBuffer, &mInputBuffer);

				if (ZSTD_isError(zstdError)) [[unlikely]]
					return DecompressionResults{
						.DecompressedByteArr{},
						.HResult = Util::ZSTD::ZSTDErrorToHRESULT(zstdError)
					};

				if (zstdError == 0)
				{
					mOperationFinished = true;
					currBlock.resize(outputBuffer.pos);
					currBlock.shrink_to_fit();
				}

				numBytesDecompressed += currBlock.size();
				dataBlockArr.push_back(std::move(currBlock));
			}

			// Now, merge all of the bytes into a single contiguous array.
			std::vector<std::byte> decompressedByteArr{};
			decompressedByteArr.resize(numBytesDecompressed);

			std::span<std::byte> copyDestSpan{ decompressedByteArr };

			for (const auto& block : dataBlockArr)
			{
				std::memcpy(copyDestSpan.data(), block.data(), block.size());
				copyDestSpan = copyDestSpan.subspan(block.size());
			}

			return DecompressionResults{
				.DecompressedByteArr{ std::move(decompressedByteArr) },
				.HResult = S_OK
			};
		}

		HRESULT ZSTDDecompressionOperation::FinishDecompressionOperation(const std::span<std::byte> destDataSpan)
		{
			const HRESULT hr = ContinueDecompressionOperation(destDataSpan);

			if (FAILED(hr)) [[unlikely]]
				return hr;

			return (mOperationFinished ? S_OK : E_NOT_SUFFICIENT_BUFFER);
		}

		bool ZSTDDecompressionOperation::IsDecompressionComplete() const
		{
			return mOperationFinished;
		}

		std::size_t ZSTDDecompressionOperation::GetZSTDBlockSize() const
		{
			return ZSTD_DStreamOutSize();
		}
	}
}