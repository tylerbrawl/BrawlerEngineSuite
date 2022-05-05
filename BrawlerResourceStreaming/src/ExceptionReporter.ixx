module;
#include <exception>
#include <string>

export module Brawler.ExceptionReporter;

export namespace Brawler
{
	class ExceptionReporter
	{
	public:
		ExceptionReporter(const std::exception& exception);

		void ReportException() const;

	private:
		const std::string mErrorStr;
	};
}