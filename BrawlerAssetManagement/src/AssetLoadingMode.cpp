module;
#include <cstdint>
#include <thread>
#include <cassert>

module Brawler.AssetManagement.AssetLoadingMode;

namespace Brawler
{
	namespace AssetManagement
	{
		std::uint32_t GetSuggestedThreadCountForAssetIORequests(const AssetLoadingMode loadingMode)
		{
			static constexpr float OPTIMIZE_FOR_RUNTIME_THREAD_COUNT_MULTIPLIER = 0.25f;
			static constexpr float OPTIMIZE_FOR_LOADING_THREAD_COUNT_MULTIPLIER = 0.6f;

			static constexpr auto THREAD_COUNT_SOLVER_LAMBDA = []<float ThreadCountMultiplier>(const std::uint32_t maxThreadCount)
			{
				const float optimalThreadCount = (static_cast<float>(maxThreadCount) * ThreadCountMultiplier);
				return std::max<std::uint32_t>(static_cast<std::size_t>(optimalThreadCount), 1);
			};

			static const std::uint32_t maxThreadCount = std::thread::hardware_concurrency();

			switch (loadingMode)
			{
			case AssetLoadingMode::MINIMAL_OVERHEAD:
				return 1;

			case AssetLoadingMode::OPTIMIZE_FOR_RUNTIME:
			{
				static const std::uint32_t optimalThreadCount = THREAD_COUNT_SOLVER_LAMBDA.operator()<OPTIMIZE_FOR_RUNTIME_THREAD_COUNT_MULTIPLIER>(maxThreadCount);
				return optimalThreadCount;
			}

			case AssetLoadingMode::OPTIMIZE_FOR_LOADING:
			{
				static const std::uint32_t optimalThreadCount = THREAD_COUNT_SOLVER_LAMBDA.operator()<OPTIMIZE_FOR_LOADING_THREAD_COUNT_MULTIPLIER>(maxThreadCount);
				return optimalThreadCount;
			}

			case AssetLoadingMode::LOAD_OR_DIE_TRYING:
				return maxThreadCount;

			default:
			{
				assert(false && "ERROR: An unidentified AssetLoadingMode was specified in a call to AssetManager::GetSuggestedThreadCountForAssetIORequests()!");
				std::unreachable();

				return 0;
			}
			}
		}
	}
}