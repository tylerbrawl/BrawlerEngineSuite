module;
#include <thread>
#include <atomic>
#include <DxDef.h>

export module Brawler.WindowMessageHandler;
import Brawler.ThreadSafeQueue;
import Brawler.Win32.CreateWindowInfo;

namespace Brawler
{
	static constexpr std::size_t PENDING_WINDOW_MESSAGE_QUEUE_SIZE = 1024;
}

export namespace Brawler
{
	class WindowMessageHandler
	{
	private:
		struct WindowCreationRequest
		{
			Win32::CreateWindowInfo CreationInfo;
			std::atomic<HWND>* HWndPtr;
		};

	public:
		WindowMessageHandler();

		~WindowMessageHandler();

		WindowMessageHandler(const WindowMessageHandler& rhs) = delete;
		WindowMessageHandler& operator=(const WindowMessageHandler& rhs) = delete;

		WindowMessageHandler(WindowMessageHandler&& rhs) noexcept = default;
		WindowMessageHandler& operator=(WindowMessageHandler&& rhs) noexcept = default;

		HWND RequestWindowCreation(Win32::CreateWindowInfo&& creationInfo);

		/// <summary>
		/// Dispatches the window messages stored within the pending message queue. The function
		/// does not return until the pending message queue reports that there are no more
		/// messages left to dispatch.
		/// 
		/// Dispatching a window message involves calling the window procedure corresponding to
		/// the window to which it is to be sent. In the Brawler Engine, all created Win32
		/// windows have the same window procedure.
		/// 
		/// If this function is called by a single thread, then it is guaranteed that window messages
		/// will be processed in the order in which they were sent by the operating system; this is
		/// true both for messages belonging to the same window and for messages belonging to
		/// different windows. This guarantee no longer holds true if this function is called 
		/// concurrently by multiple threads.
		/// 
		/// However, assuming that window message handlers are aware of this and synchronize shared 
		/// data as needed, this function is safe to call from multiple threads. Unfortunately, there 
		/// is no way to know what order messages were sent in. For that reason, and because there are 
		/// not likely to be many windows created at once, multi-threaded window message processing is
		/// generally not recommended.
		/// </summary>
		void DispatchWindowMessages();

	private:
		void RunMessageLoop();

		void FulfillWindowCreationRequest(const WindowCreationRequest& creationRequest) const;

	private:
		std::atomic<bool> mKeepGoing;
		Brawler::ThreadSafeQueue<MSG, PENDING_WINDOW_MESSAGE_QUEUE_SIZE> mPendingMessageQueue;
		std::uint32_t mWindowHandlerThreadID;

		/// <summary>
		/// As much as I would love to not have to create any additional threads, the Win32
		/// API has this pretty crappy restriction where the only thread which can get the
		/// messages for a given window is the same thread which created said window. So,
		/// unless we want to do some pretty long synchronization waits during window message
		/// handling to ensure that every thread in the WorkerThreadPool along with the main
		/// thread get the chance to dispatch window messages, we need a separate thread
		/// for creating windows and extracting messages from the message loops of said
		/// windows.
		/// </summary>
		std::jthread mWindowHandlerThread;
	};
}