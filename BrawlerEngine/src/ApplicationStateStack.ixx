module;
#include <vector>
#include <memory>

export module Brawler.ApplicationStateStack;
import Brawler.ThreadSafeQueue;
import Brawler.I_ApplicationState;
import Brawler.ApplicationStateStackRequestBundle;

namespace Brawler
{
	namespace IMPL
	{
		// We really shouldn't need that big of a state stack request queue.
		static constexpr std::size_t REQUEST_QUEUE_SIZE = 8;
	}
}

export namespace Brawler
{
	class ApplicationStateStack
	{
	public:
		ApplicationStateStack();

		ApplicationStateStack(const ApplicationStateStack& rhs) = delete;
		ApplicationStateStack& operator=(const ApplicationStateStack& rhs) = delete;

		ApplicationStateStack(ApplicationStateStack&& rhs) noexcept = default;
		ApplicationStateStack& operator=(ApplicationStateStack&& rhs) noexcept = default;

		/// <summary>
		/// Submits the specified ApplicationStateStackRequestBundle to the request queue for
		/// this ApplicationStateStack. The actual requests are executed just before the
		/// states within the ApplicationStateStack receive their next update.
		/// 
		/// This function is thread safe, and all of the requests made within a single bundle
		/// are guaranteed to be executed in the order which they were added to the bundle.
		/// 
		/// Bundles themselves are processed in an FIFO manner. However, this function
		/// can be called by multiple threads. Thus, if you need to execute a set of requests
		/// sequentially, then it is best to pack them into a single bundle. If this cannot
		/// be done, then some form of external synchronization is required (e.g., submitting
		/// the requests all on the same thread).
		/// </summary>
		/// <param name="requestBundle"></param>
		void SubmitStateStackRequestBundle(ApplicationStateStackRequestBundle&& requestBundle);

		/// <summary>
		/// Calls I_ApplicationState::Update for every state in the state stack in top-down
		/// order until either one state refuses to let other states be updated or all of the
		/// states are updated.
		/// </summary>
		/// <param name="dt">
		/// - The time (in seconds) which is to be used as the timestep for state updates.
		/// </param>
		void Update(const float dt);

	private:
		void ProcessStateStackRequests();
		void ProcessRequestBundle(const ApplicationStateStackRequestBundle& requestBundle);

	private:
		std::vector<std::unique_ptr<I_ApplicationState>> mStateStack;
		Brawler::ThreadSafeQueue<ApplicationStateStackRequestBundle, IMPL::REQUEST_QUEUE_SIZE> mRequestQueue;
	};
}