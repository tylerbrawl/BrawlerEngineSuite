module;
#include <DxDef.h>

export module Brawler.Win32.WindowMessage;

export namespace Brawler
{
	namespace Win32
	{
		struct WindowMessage
		{
			UINT Msg;
			WPARAM WParam;
			LPARAM LParam;
		};

		struct WindowMessageResult
		{
			bool MessageHandled;
			LRESULT Result;
		};

		constexpr WindowMessageResult HandledMessageResult(const LRESULT result)
		{
			return WindowMessageResult{ true, result };
		}

		consteval WindowMessageResult UnhandledMessageResult()
		{
			return WindowMessageResult{ false, 0 };
		}
	}
}