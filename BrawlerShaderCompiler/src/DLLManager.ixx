module;
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <optional>
#include "DxDef.h"

export module Brawler.DLLManager;

namespace Brawler
{
	namespace Win32
	{
		namespace IMPL
		{
			struct ModuleDeleter
			{
				void operator()(HMODULE hModule) const
				{
					FreeLibrary(hModule);
				}
			};
		}

		using SafeModule = std::unique_ptr<std::remove_pointer_t<HMODULE>, IMPL::ModuleDeleter>;
	}
}

export namespace Brawler
{
	namespace Win32
	{
		enum class ModuleSearchPath
		{
			SYSTEM32 = LOAD_LIBRARY_SEARCH_SYSTEM32,
			APPLICATION_LIBRARY_DIRECTORY = LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32
		};

		class DLLManager
		{
		private:
			DLLManager();

		public:
			~DLLManager();

			DLLManager(const DLLManager& rhs) = delete;
			DLLManager& operator=(const DLLManager& rhs) = delete;

			DLLManager(DLLManager&& rhs) noexcept = default;
			DLLManager& operator=(DLLManager&& rhs) noexcept = default;

			static DLLManager& GetInstance();

			bool LoadModule(const std::wstring_view moduleName, const ModuleSearchPath searchPath);
			std::optional<HMODULE> GetModule(const std::wstring_view moduleName) const;

		private:
			DLL_DIRECTORY_COOKIE mDirCookie;
			mutable std::mutex mCritSection;
			std::unordered_map<std::wstring_view, SafeModule> mModuleMap;
		};
	}
}