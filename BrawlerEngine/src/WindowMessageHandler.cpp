module;
#include <thread>
#include <optional>
#include <atomic>
#include <DxDef.h>

module Brawler.WindowMessageHandler;
import Brawler.Application;
import Brawler.Manifest;
import Brawler.NZStringView;
import Util.General;

namespace
{
	enum class CustomWindowMessage : std::uint32_t
	{
		/// <summary>
		/// WM_CREATEWINDOW: Either the main thread or the WorkerThread is requesting that
		/// this thread create a new window.
		///
		/// wParam: This is a pointer to a WindowMessageHandler::WindowCreationRequest
		/// instance which both describes the window which needs to be created and points
		/// to a std::atomic&lt;HWND&gt; to allow the calling thread to get the result back
		/// in a thread-safe manner.
		///
		/// lParam: This parameter is reserved, and must be 0.
		/// </summary>
		WM_CREATEWINDOW = WM_USER,

		/// <summary>
		/// WM_SHUTDOWN: One of the other threads of the application (likely the main thread)
		/// is informing the mWindowHandlerThread that the program is terminating. Once this
		/// message is received, mWindowHandlerThread will terminate.
		/// 
		/// wParam: This is the exit code for the application. It will ultimately become the
		/// return value of the wWinMain() function.
		/// 
		/// lParam: This parameter is reserved, and must be 0.
		/// </summary>
		WM_SHUTDOWN
	};
}

namespace Brawler
{
	WindowMessageHandler::WindowMessageHandler() :
		mKeepGoing(true),
		mPendingMessageQueue(),
		mWindowHandlerThreadID(0),
		mWindowHandlerThread()
	{
		// Wait until we can get the thread ID of mWindowHandlerThread.
		std::atomic<bool> threadIDInitialized{ false };
		mWindowHandlerThread = std::jthread{ [this, &threadIDInitialized] ()
		{
			mWindowHandlerThreadID = GetCurrentThreadId();

			// According to the MSDN, not every thread gets a message queue. To get a message
			// queue created for a thread, it must call "one of the specific user functions."
			//
			// I'm not sure which functions they are referring to, but to ensure correctness,
			// mWindowHandlerThread should have a message queue before any window creation
			// requests are made.
			//
			// I'm just going to call PeekMessage() here and hope for the best.
			{
				MSG dummyMsg{};
				PeekMessage(
					&dummyMsg,
					nullptr,
					0,
					0,
					PM_NOREMOVE
				);
			}

			threadIDInitialized.store(true, std::memory_order::relaxed);
			threadIDInitialized.notify_one();

			RunMessageLoop();
		} };

		threadIDInitialized.wait(false, std::memory_order::relaxed);
	}

	WindowMessageHandler::~WindowMessageHandler()
	{
		// Although the destructor of std::jthread calls std::jthread::request_stop() before
		// calling std::jthread::join(), mWindowHandlerThread is still going to be waiting
		// within GetMessage() until an actual window message arrives. To prevent a deadlock,
		// then, we need to issue a stop request ourselves before sending a custom window message
		// which will get the thread to exit.

		// We wait to call std::jthread::request_stop() until after we get the thread ID, just to
		// be safe.
		mWindowHandlerThread.request_stop();

		BOOL postThreadMessageResult = FALSE;
		do
		{
			postThreadMessageResult = PostThreadMessage(
				windowHandlerThreadID,
				std::to_underlying(CustomWindowMessage::WM_SHUTDOWN),
				0,
				0
			);

			if (!postThreadMessageResult) [[unlikely]]
			{
				switch (GetLastError())
				{
				case ERROR_INVALID_THREAD_ID:
				{
					// I guess mWindowHandlerThread already left, then? In that case, we treat this as
					// a non-error.
					postThreadMessageResult = TRUE;
					break;
				}

				case ERROR_NOT_ENOUGH_QUOTA:
				{
					// The message queue was full, so just try again.
					break;
				}

				default: [[unlikely]]
				{
					// Oh, crap... Now what? I guess we just have to kill the process if we don't want to
					// deadlock.
					std::terminate();
					break;
				}
				}
			}
		} while (!postThreadMessageResult);
	}

	HWND WindowMessageHandler::RequestWindowCreation(Win32::CreateWindowInfo&& creationInfo)
	{
		std::atomic<HWND> hWndResult{ nullptr };
		WindowCreationRequest creationRequest{
			.CreationInfo{ std::move(creationInfo) },
			.HWndPtr = &hWndResult
		};

		const WPARAM createWindowWParam = reinterpret_cast<WPARAM>(&creationRequest);
		BOOL postThreadMessageResult = FALSE;

		do
		{
			postThreadMessageResult = PostThreadMessage(
				mWindowHandlerThreadID,
				std::to_underlying(CustomWindowMessage::WM_CREATEWINDOW),
				createWindowWParam,
				0
			);

			if (!postThreadMessageResult) [[unlikely]]
			{
				const std::int32_t lastError = GetLastError();
				
				switch (GetLastError())
				{
				case ERROR_NOT_ENOUGH_QUOTA:
				{
					// The message queue was full, so we try again.
					break;
				}

				default: [[unlikely]]
				{
					Util::General::CheckHRESULT(lastError);
					postThreadMessageResult = true;

					break;
				}
				}
			}
		} while (!postThreadMessageResult);

		// Wait until mWindowHandlerThread can create the window for us. If we want to, we can make
		// this an asynchronous function. For now, however, it doesn't seem like it would be worth
		// the effort, considering that we aren't going to be creating too many windows in the general
		// case.
		hWndResult.wait(nullptr, std::memory_order::relaxed);

		return hWndResult.load(std::memory_order::relaxed);
	}

	void WindowMessageHandler::DispatchWindowMessages()
	{
		std::optional<MSG> acquiredMsg{ mPendingMessageQueue.TryPop() };

		while (acquiredMsg.has_value())
		{
			DispatchMessage(&(*acquiredMsg));
			acquiredMsg = mPendingMessageQueue.TryPop();
		}
	}

	void WindowMessageHandler::RunMessageLoop()
	{
		const std::stop_token stopToken{ mWindowHandlerThread.get_stop_token() };
		MSG currMsg{};

		while (!stopToken.stop_requested())
		{
			// We want mWindowHandlerThread to do as little work as possible to ensure that we
			// don't reduce the amount of CPU time dedicated to both the main thread and the
			// WorkerThreads. Thankfully, the Win32 API function GetMessage() will efficiently
			// suspend the calling thread until a queued message arrives.
			
			const std::int32_t getMessageResult = static_cast<std::int32_t>(GetMessage(
				&currMsg,
				nullptr,
				0,
				0
			));

			// If we encounter an error, try to terminate the process gracefully.
			if (getMessageResult == -1) [[unlikely]]
			{
				mWindowHandlerThread.request_stop();
				Brawler::GetApplication().Terminate(GetLastError());

				return;
			}

			// Check for messages which this thread must actually process before offloading
			// the work to the other, more important threads.
			switch (currMsg.message)
			{
			case std::to_underlying(CustomWindowMessage::WM_CREATEWINDOW):
			{
				const WindowCreationRequest* const creationRequestPtr = reinterpret_cast<const WindowCreationRequest*>(currMsg.wParam);
				FulfillWindowCreationRequest(*creationRequestPtr);

				break;
			}

			case std::to_underlying(CustomWindowMessage::WM_SHUTDOWN):
			{
				// Post the WM_QUIT message to this thread's message queue.
				PostQuitMessage(Brawler::GetApplication().GetExitCode());

				break;
			}

			case WM_QUIT:
			{
				mWindowHandlerThread.request_stop();
				Brawler::GetApplication().Terminate(currMsg.wParam);

				break;
			}

			default: [[likely]]
			{
				TranslateMessage(&currMsg);
				
				// Push the window message to the dispatched message queue. Other threads will
				// then be able to process the window messages concurrently.
				while (!mPendingMessageQueue.PushBack(std::move(currMsg)))
					std::this_thread::yield();

				break;
			}
			}
		}
	}

	void WindowMessageHandler::FulfillWindowCreationRequest(const WindowCreationRequest& creationRequest) const
	{
		const Win32::CreateWindowInfo& creationInfo{ creationRequest.CreationInfo };
		const HWND hWnd = CreateWindowEx(
			creationInfo.WindowStyleEx,
			Brawler::Manifest::WINDOW_CLASS_NAME_STR.C_Str(),
			Brawler::Manifest::APPLICATION_NAME.C_Str(),
			creationInfo.WindowStyleEx,
			creationInfo.WindowStartCoordinates.x,
			creationInfo.WindowStartCoordinates.y,
			static_cast<std::uint32_t>(creationInfo.WindowSize.x),
			static_cast<std::uint32_t>(creationInfo.WindowSize.y),
			nullptr,
			nullptr,
			Brawler::GetApplication().GetInstanceHandle(),
			nullptr
		);

		creationRequest.HWndPtr->store(hWnd, std::memory_order::relaxed);
		creationRequest.HWndPtr->notify_one();
	}
}