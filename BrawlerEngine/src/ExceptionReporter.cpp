module;
#include "DxDef.h"

module Brawler.ExceptionReporter;
import Util.General;

namespace Brawler
{
	ExceptionReporter::ExceptionReporter(const std::exception& exception) :
		mErrorStr()
	{
		// We will abstract the details for Release builds.

#ifdef _DEBUG
		mErrorStr = std::wstring{ L"ERROR: The following unhandled exception occurred:\n" + Util::General::StringToWString(exception.what()) };
#else
		mErrorStr = std::wstring{ L"ERROR: A fatal error has occurred. The process will now be terminated." };
#endif // _DEBUG
	}

	void ExceptionReporter::ReportException() const
	{
		MessageBox(
			nullptr,
			mErrorStr.c_str(),
			L"Fatal Error",
			MB_OK
		);
	}
}