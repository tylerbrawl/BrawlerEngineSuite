module;
#include <cstddef>
#include <optional>
#include <zstd.h>

export module Brawler.ZSTDContext:ZSTDContextQueue;
import Brawler.ThreadSafeQueue;
import :UnderlyingZSTDContextTypes;

namespace Brawler
{
	static constexpr std::size_t COMPRESSION_CONTEXT_QUEUE_SIZE = 100;
	static constexpr std::size_t DECOMPRESSION_CONTEXT_QUEUE_SIZE = 100;
}

export namespace Brawler
{
	class ZSTDContextQueue final
	{
	private:
		ZSTDContextQueue() = default;

	public:
		~ZSTDContextQueue() = default;

		ZSTDContextQueue(const ZSTDContextQueue& rhs) = delete;
		ZSTDContextQueue& operator=(const ZSTDContextQueue& rhs) noexcept = delete;

		ZSTDContextQueue(ZSTDContextQueue&& rhs) noexcept = delete;
		ZSTDContextQueue& operator=(ZSTDContextQueue&& rhs) noexcept = delete;

		static ZSTDContextQueue& GetInstance();

		template <typename ContextType>
			requires IsZSTDContextType<ContextType>
		ContextType GetZSTDContext();

		template <typename ContextType>
			requires IsZSTDContextType<ContextType>
		void ReturnZSTDContext(ContextType&& context);

	private:
		ZSTDCompressionContextIMPL GetZSTDCompressionContext();
		ZSTDDecompressionContextIMPL GetZSTDDecompressionContext();

	private:
		Brawler::ThreadSafeQueue<ZSTDCompressionContextIMPL, COMPRESSION_CONTEXT_QUEUE_SIZE> mCompressionContextQueue;
		Brawler::ThreadSafeQueue<ZSTDDecompressionContextIMPL, DECOMPRESSION_CONTEXT_QUEUE_SIZE> mDecompressionContextQueue;
	};
}

// ---------------------------------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	ZSTDContextQueue& ZSTDContextQueue::GetInstance()
	{
		static ZSTDContextQueue instance{};
		return instance;
	}

	template <typename ContextType>
		requires IsZSTDContextType<ContextType>
	ContextType ZSTDContextQueue::GetZSTDContext()
	{
		if constexpr (std::is_same_v<ContextType, ZSTDCompressionContextIMPL>)
			return GetZSTDCompressionContext();
		else
			return GetZSTDDecompressionContext();
	}

	template <typename ContextType>
		requires IsZSTDContextType<ContextType>
	void ZSTDContextQueue::ReturnZSTDContext(ContextType&& context)
	{
		// If we can't push the context onto the queue, then we can just let it get destroyed.
		if constexpr (std::is_same_v<ContextType, ZSTDCompressionContextIMPL>)
			std::ignore = mCompressionContextQueue.PushBack(std::move(context));
		else
			std::ignore = mDecompressionContextQueue.PushBack(std::move(context));
	}

	ZSTDCompressionContextIMPL ZSTDContextQueue::GetZSTDCompressionContext()
	{
		std::optional<ZSTDCompressionContextIMPL> optionalContext{ mCompressionContextQueue.TryPop() };

		if (optionalContext.has_value())
			return std::move(*optionalContext);

		return ZSTDCompressionContextIMPL{ ZSTD_createCCtx() };
	}

	ZSTDDecompressionContextIMPL ZSTDContextQueue::GetZSTDDecompressionContext()
	{
		std::optional<ZSTDDecompressionContextIMPL> optionalContext{ mDecompressionContextQueue.TryPop() };

		if (optionalContext.has_value())
			return std::move(*optionalContext);

		return ZSTDDecompressionContextIMPL{ ZSTD_createDCtx() };
	}
}