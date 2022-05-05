module;
#include "DxDef.h"

export module Win32.WindowMessage;

export namespace Win32
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

	constexpr WindowMessageResult HandledMessageResult(LRESULT result)
	{
		return WindowMessageResult{ true, result };
	}

	constexpr WindowMessageResult UnhandledMessageResult()
	{
		return WindowMessageResult{ false, 0 };
	}
}