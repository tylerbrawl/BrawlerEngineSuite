#include <stdexcept>
#include <thread>

import Brawler.Application;
import Brawler.ExceptionReporter;

int main(int argc, char* argv[])
{
	try
	{
		Brawler::Application app{};
		app.Run();
	}
	catch (const std::exception& e)
	{
		Brawler::ExceptionReporter reporter{ e };
		reporter.ReportException();
	}

	return 0;
}