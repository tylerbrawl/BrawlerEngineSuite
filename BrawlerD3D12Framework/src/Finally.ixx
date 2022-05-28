module;
#include <utility>

export module Brawler.Finally;
import Brawler.Functional;

export namespace Brawler
{
	template <Brawler::Function<void> CallbackType>
	class Finally
	{
	public:
		explicit Finally(CallbackType&& callback);

		~Finally();

		Finally(const Finally& rhs) = delete;
		Finally& operator=(const Finally& rhs) = delete;

		Finally(Finally&& rhs) noexcept = delete;
		Finally& operator=(Finally&& rhs) noexcept = delete;

	private:
		CallbackType mCallback;
	};
}

// -------------------------------------------------------------------------------------------

namespace Brawler
{
	template <Brawler::Function<void> CallbackType>
	Finally<CallbackType>::Finally(CallbackType&& callback) :
		mCallback(std::move(callback))
	{}

	template <Brawler::Function<void> CallbackType>
	Finally<CallbackType>::~Finally()
	{
		mCallback();
	}
}