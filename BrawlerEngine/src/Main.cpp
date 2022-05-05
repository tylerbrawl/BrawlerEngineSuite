#include "DxDef.h"
import Brawler.Application;
import Brawler.ExceptionReporter;

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance, 
	_In_opt_ HINSTANCE hPrevInstance, 
	_In_ LPWSTR lpCmdLine, 
	_In_ int nCmdShow
)
{
	try
	{
		Brawler::Application app{ hInstance, nCmdShow };
		app.Run();
	}
	catch (const std::exception& e)
	{
		Brawler::ExceptionReporter reporter{ e };
		reporter.ReportException();

		return 1;
	}

	return 0;
}