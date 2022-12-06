module;
#include <cassert>
#include <optional>
#include <span>
#include <memory>
#include <ranges>

module Brawler.ApplicationStateStack;
import Brawler.ApplicationStateStackRequestBundle;
import Brawler.IMPL.ApplicationStateStackRequest;
import Brawler.ApplicationStateMap;
import Brawler.ApplicationStateID;

namespace Brawler
{
	ApplicationStateStack::ApplicationStateStack() :
		mStateStack(),
		mRequestQueue()
	{}

	void ApplicationStateStack::SubmitStateStackRequestBundle(ApplicationStateStackRequestBundle&& requestBundle)
	{
		[[maybe_unused]] const bool result = mRequestQueue.PushBack(std::move(requestBundle));
		assert(result && "ERROR: The ApplicationStateStack request queue could not contain all of the submitted bundles before the next update tick!");
	}

	void ApplicationStateStack::Update(const float dt)
	{
		// First, process any state stack requests which we have received.
		ProcessStateStackRequests();
		
		// Update the states within the stack from top to bottom. Stop when a state tells us
		// that states below it should not be updated.
		for (auto& state : mStateStack | std::views::reverse)
		{
			if (!state->Update(dt))
				break;
		}
	}

	void ApplicationStateStack::ProcessStateStackRequests()
	{
		std::optional<ApplicationStateStackRequestBundle> extractedBundle{};

		do
		{
			extractedBundle = mRequestQueue.TryPop();

			if (extractedBundle.has_value())
				ProcessRequestBundle(*extractedBundle);
		} while (extractedBundle.has_value());
	}

	void ApplicationStateStack::ProcessRequestBundle(const ApplicationStateStackRequestBundle& requestBundle)
	{
		const std::span<const IMPL::ApplicationStateStackRequest> requestArr{ requestBundle.GetStateStackRequests() };

		for (const auto& request : requestArr)
		{
			switch (request.Type)
			{
			case IMPL::ApplicationStateStackRequestType::PUSH:
			{
				assert(request.StatePtr != nullptr);
				mStateStack.push_back(std::move(request.StatePtr));

				break;
			}

			case IMPL::ApplicationStateStackRequestType::POP:
			{
				assert(!mStateStack.empty() && "ERROR: An attempt was made to pop an empty ApplicationStateStack! (Do we have a race condition somewhere?)");
				mStateStack.pop_back();

				break;
			}

			case IMPL::ApplicationStateStackRequestType::CLEAR:
			{
				mStateStack.clear();

				break;
			}

			default:
			{
				assert(false && "ERROR: An unhandled IMPL::ApplicationStateStackRequestType was detected! (See ApplicationStateStack::ProcessRequestBundle() in ApplicationStateStack.cpp.)");
				std::unreachable();

				break;
			}
			}
		}
	}
}