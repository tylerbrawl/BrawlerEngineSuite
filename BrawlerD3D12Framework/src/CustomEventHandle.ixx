module;
#include <utility>

export module Brawler.CustomEventHandle;
import Brawler.I_EventHandle;
import Brawler.Functional;

export namespace Brawler
{
	template <Brawler::Function<bool> Callback>
	class CustomEventHandle final : public I_EventHandle
	{
	public:
		// Make the constructor implicit to allow for easy creation. We do this only because
		// it makes sense in this scenario.
		CustomEventHandle(Callback&& callback);

		CustomEventHandle(const CustomEventHandle& rhs) = delete;
		CustomEventHandle& operator=(const CustomEventHandle& rhs) = delete;

		CustomEventHandle(CustomEventHandle&& rhs) noexcept = default;
		CustomEventHandle& operator=(CustomEventHandle&& rhs) noexcept = default;

		bool IsEventCompleted() const override;

	private:
		Callback mCallback;
	};
}

// ---------------------------------------------------------------------------------------------------

namespace Brawler
{
	template <Brawler::Function<bool> Callback>
	CustomEventHandle<Callback>::CustomEventHandle(Callback&& callback) :
		mCallback(std::move(callback))
	{}

	template <Brawler::Function<bool> Callback>
	bool CustomEventHandle<Callback>::IsEventCompleted() const
	{
		return mCallback();
	}
}