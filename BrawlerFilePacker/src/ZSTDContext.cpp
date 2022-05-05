module;
#include <span>
#include <stdexcept>
#include <zstd.h>

module Brawler.ZSTDContext;
import Brawler.ZSTDFrame;
import Util.Engine;

namespace Brawler
{
	ZSTDContext::ZSTDContext() :
		mCompressionContextPtr(ZSTD_createCCtx())
	{}

	ZSTDContext::~ZSTDContext()
	{
		DeleteCompressionContext();
	}

	ZSTDContext::ZSTDContext(ZSTDContext&& rhs) noexcept :
		mCompressionContextPtr(rhs.mCompressionContextPtr)
	{
		rhs.mCompressionContextPtr = nullptr;
	}

	ZSTDContext& ZSTDContext::operator=(ZSTDContext&& rhs) noexcept
	{
		DeleteCompressionContext();

		mCompressionContextPtr = rhs.mCompressionContextPtr;
		rhs.mCompressionContextPtr = nullptr;

		return *this;
	}

	ZSTDFrame ZSTDContext::CompressData(const std::span<std::uint8_t> byteArr) const
	{
		const std::size_t frameSize = ZSTD_compressBound(byteArr.size_bytes());
		std::vector<std::uint8_t> frameByteArr{};
		frameByteArr.resize(frameSize);

		std::size_t compressionResult = ZSTD_compressCCtx(
			mCompressionContextPtr,
			frameByteArr.data(),
			frameByteArr.size(),
			byteArr.data(),
			byteArr.size_bytes(),
			Util::Engine::GetZSTDCompressionLevel()
		);

		if (ZSTD_isError(compressionResult)) [[unlikely]]
			throw std::runtime_error{ std::string{ "ERROR: ZSTD failed to compress a frame with the following error: " } + std::string{ ZSTD_getErrorName(compressionResult) } };

		// ZSTD_compressBound() is an upper-bound on the memory required. Usually, we don't
		// need that much memory.
		if (compressionResult < frameByteArr.size()) [[likely]]
		{
			frameByteArr.resize(compressionResult);
			frameByteArr.shrink_to_fit();
		}

		return ZSTDFrame{ std::move(frameByteArr) };
	}

	void ZSTDContext::DeleteCompressionContext()
	{
		if (mCompressionContextPtr != nullptr)
		{
			ZSTD_freeCCtx(mCompressionContextPtr);
			mCompressionContextPtr = nullptr;
		}
	}
}