module;

export module Brawler.AssetManagement.Win32AssetIORequestHandler;
import Brawler.AssetManagement.I_AssetIORequestHandler;

export namespace Brawler
{
	namespace AssetManagement
	{
		class Win32AssetIORequestHandler final : public I_AssetIORequestHandler
		{
		public:
			Win32AssetIORequestHandler() = default;

			Win32AssetIORequestHandler(const Win32AssetIORequestHandler& rhs) = delete;
			Win32AssetIORequestHandler& operator=(const Win32AssetIORequestHandler& rhs) = delete;

			Win32AssetIORequestHandler(Win32AssetIORequestHandler&& rhs) noexcept = default;
			Win32AssetIORequestHandler& operator=(Win32AssetIORequestHandler&& rhs) noexcept = default;
		};
	}
}