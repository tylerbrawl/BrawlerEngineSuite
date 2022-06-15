module;

export module Brawler.Win32EventHandle;
import Brawler.I_EventHandle;
import Brawler.Win32.SafeHandle;

export namespace Brawler
{
	class Win32EventHandle final : public I_EventHandle
	{
	public:
		explicit Win32EventHandle(Win32::SafeHandle&& hEvent);

		Win32EventHandle(const Win32EventHandle& rhs) = delete;
		Win32EventHandle& operator=(const Win32EventHandle& rhs) = delete;

		Win32EventHandle(Win32EventHandle&& rhs) noexcept = default;
		Win32EventHandle& operator=(Win32EventHandle&& rhs) noexcept = default;

		bool IsEventCompleted() const override;

	private:
		Win32::SafeHandle mHEvent;
	};
}