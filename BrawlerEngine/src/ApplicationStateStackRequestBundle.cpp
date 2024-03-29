module;
#include <vector>
#include <span>

module Brawler.ApplicationStateStackRequestBundle;

namespace Brawler
{
	ApplicationStateStackRequestBundle::ApplicationStateStackRequestBundle(const std::size_t requestCount) :
		mRequestArr()
	{
		Reserve(requestCount);
	}

	void ApplicationStateStackRequestBundle::Reserve(const std::size_t requestCount)
	{
		mRequestArr.reserve(requestCount);
	}

	void ApplicationStateStackRequestBundle::RequestStatePop()
	{
		mRequestArr.push_back(IMPL::ApplicationStateStackRequest{
			.Type = IMPL::ApplicationStateStackRequestType::POP,
			.StateID = ApplicationStateID::COUNT_OR_ERROR
		});
	}

	void ApplicationStateStackRequestBundle::RequestStateClear()
	{
		mRequestArr.push_back(IMPL::ApplicationStateStackRequest{
			.Type = IMPL::ApplicationStateStackRequestType::CLEAR,
			.StateID = ApplicationStateID::COUNT_OR_ERROR
		});
	}

	std::span<const IMPL::ApplicationStateStackRequest> ApplicationStateStackRequestBundle::GetStateStackRequests() const
	{
		return mRequestArr;
	}
}