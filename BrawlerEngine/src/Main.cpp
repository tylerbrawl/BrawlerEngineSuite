#include "DxDef.h"
import Brawler.Application;
import Util.Win32;
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
		Util::Win32::InitializeWin32Components();

		Brawler::Application app{ hInstance, nCmdShow };
		app.Run();

		return app.GetExitCode();
	}
	catch (const std::exception& e)
	{
		Brawler::ExceptionReporter reporter{ e };
		reporter.ReportException();

		return 1;
	}
}