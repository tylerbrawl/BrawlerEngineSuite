module;
#include <memory>

export module Brawler.MainWindowManager;
import Brawler.AppWindow;

export namespace Brawler
{
	class MainWindowManager
	{
	public:
		MainWindowManager() = default;

		MainWindowManager(const MainWindowManager& rhs) = delete;
		MainWindowManager& operator=(const MainWindowManager& rhs) = delete;

		MainWindowManager(MainWindowManager&& rhs) noexcept = default;
		MainWindowManager& operator=(MainWindowManager&& rhs) noexcept = default;

		void CreateMainWindow();

		AppWindow& GetMainAppWindow();
		const AppWindow& GetMainAppWindow() const;

	private:
		std::unique_ptr<AppWindow> mMainWndPtr;
	};
}