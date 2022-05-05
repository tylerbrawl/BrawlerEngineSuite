module;
#include <string>

module Brawler.ExceptionReporter;
import Util.Win32;

namespace Brawler
{
	ExceptionReporter::ExceptionReporter(const std::exception& exception) :
		// For the file packer, we will *NOT* abstract the details for Release builds.
		mErrorStr(exception.what())
	{}

	void ExceptionReporter::ReportException() const
	{
		Util::Win32::WriteFormattedConsoleMessage(mErrorStr, Util::Win32::ConsoleFormat::CRITICAL_FAILURE);
	}
}