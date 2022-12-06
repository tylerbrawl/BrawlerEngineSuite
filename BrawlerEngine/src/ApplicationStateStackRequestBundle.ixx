module;
#include <vector>
#include <span>

export module Brawler.ApplicationStateStackRequestBundle;
import Brawler.IMPL.ApplicationStateStackRequest;
import Brawler.I_ApplicationState;
import Brawler.ApplicationStateMap;

namespace Brawler
{
	class ApplicationStateStack;
}

export namespace Brawler
{
	class ApplicationStateStackRequestBundle
	{
	private:
		friend class ApplicationStateStack;

	public:
		explicit ApplicationStateStackRequestBundle(const std::size_t requestCount = 1);

		/// <summary>
		/// Allocates space for requestCount StateStackRequests.
		/// </summary>
		/// <param name="requestCount"></param>
		void Reserve(const std::size_t requestCount);

		/// <summary>
		/// Make a request to the ApplicationStateStack instance to push a new state onto the
		/// stack.
		/// 
		/// Although all functions which make state stack requests are thread-safe, the requests
		/// are processed in an FIFO manner. Thus, if you want to guarantee serial execution of
		/// a set of request bundles, then these bundles should be submitted on a single thread or
		/// synchronized externally.
		/// </summary>
		/// <typeparam name="T">
		/// - The type of I_ApplicationState which is to be pushed onto the ApplicationStateStack.
		/// </typeparam>
		template <typename T, typename... Args>
			requires std::derived_from<T, I_ApplicationState> && std::constructible_from<T, Args...>
		void RequestStatePush(Args&&... args);

		/// <summary>
		/// Make a request to the ApplicationStateStack instance to pop the top-level I_ApplicationState
		/// from the stack.
		/// 
		/// Although all functions which make state stack requests are thread-safe, the requests
		/// are processed in an FIFO manner. Thus, if you want to guarantee serial execution of
		/// a set of request bundles, then these bundles should be submitted on a single thread or
		/// synchronized externally.
		/// </summary>
		void RequestStatePop();

		/// <summary>
		/// Make a request to the ApplicationStateStack to clear its entire stack of states. This should
		/// probably be followed by a call to ApplicationStateStackRequestBundle::RequestStatePush().
		/// 
		/// Although all functions which make state stack requests are thread-safe, the requests
		/// are processed in an FIFO manner. Thus, if you want to guarantee serial execution of
		/// a set of request bundles, then these bundles should be submitted on a single thread or
		/// synchronized externally.
		/// </summary>
		void RequestStateClear();

	private:
		std::span<const IMPL::ApplicationStateStackRequest> GetStateStackRequests() const;

	private:
		std::vector<IMPL::ApplicationStateStackRequest> mRequestArr;
	};
}

// --------------------------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <typename T>
		requires std::derived_from<T, I_ApplicationState> && !std::is_same_v<T, I_ApplicationState>
	void ApplicationStateStackRequestBundle::RequestStatePush(Args&&... args)
	{
		constexpr ApplicationStateID STATE_ID = GetApplicationStateID<T>();

		mRequestArr.push_back(IMPL::ApplicationStateStackRequest{
			.Type = IMPL::ApplicationStateStackRequestType::PUSH,
			.StatePtr{ std::make_unique<T>(std::forward<Args>(args)...) }
		});
	}
}