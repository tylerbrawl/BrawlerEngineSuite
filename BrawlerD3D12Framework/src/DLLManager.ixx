module;
#include <mutex>
#include <unordered_map>
#include <string_view>
#include <optional>
#include <filesystem>
#include "DxDef.h"

export module Brawler.Win32.DLLManager;
import Brawler.Win32.SafeModule;

export namespace Brawler
{
	namespace Win32
	{
		class DLLManager final
		{
		private:
			enum class DLLLoadType
			{
				GET_ONLY_EXISTING_MODULES,
				GET_EXISTING_OR_LOAD_NEW_MODULES
			};

		private:
			DLLManager() = default;

		public:
			DLLManager(const DLLManager& rhs) = delete;
			DLLManager& operator=(const DLLManager& rhs) = delete;

			DLLManager(DLLManager&& rhs) noexcept;
			DLLManager& operator=(DLLManager&& rhs) noexcept;

			static DLLManager& GetInstance();

			/// <summary>
			/// Retrieves the HMODULE referred to by the DLL module file specified by moduleFilePath. The behavior
			/// of this function differs depending on whether moduleFilePath specifies only a file name
			/// or a relative or absolute path. (If moduleFilePath has any "\" characters in it, then it
			/// is considered a path.)
			/// 
			///   - If moduleFilePath specifies a path, then the function retrieves the HMODULE of the
			///     file located exactly at moduleFilePath. If this module was not already loaded, then it
			///     is loaded by this function. Modules with the same name but in a different path are *NOT*
			///     loaded or returned by this function.
			/// 
			///   - If moduleFilePath specifies only a file name, then the function does a search for the module.
			///     The search is done in the following order:
			/// 
			///       1. The System32 directory. Files in this directory are unlikely to be tampered with by users.
			///          Thus, searching this directory first is usually the safest bet to ensure DLL consistency.
			/// 
			///       2. The standard Win32 search path. For details on the path taken, refer to the MSDN at
			///          https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order#search-order-for-desktop-applications.
			/// 
			/// In both cases, if the function is able to load the correct module, then the corresponding HMODULE
			/// is returned. Otherwise, the returned std::optional instance is empty.
			/// 
			/// *NOTE*: Do *NOT* call FreeLibrary() on HMODULE instances returned by this function. Manually loaded
			/// modules are maintained by the DLLManager instance. If you want to unload/free a loaded library, call
			/// DLLManager::UnloadModule(), instead.
			/// </summary>
			/// <param name="moduleFilePath">
			/// - The std::filesystem::path referring to the module which is to be loaded. The function differs
			///   based on whether or not moduleFilePath is a file name or a relative or absolute path. For details,
			///   refer to the summary.
			/// </param>
			/// <returns>
			/// If the DLL module corresponding to moduleFilePath was successfully loaded, then the returned std::optional
			/// instance has a value, and this value is the HMODULE corresponding to the loaded module specified by
			/// moduleFilePath. See the summary for details as to how the function differs based on whether
			/// moduleFilePath is a file name or a relative or absolute path.
			/// 
			/// Otherwise, if the module could not be loaded, then the returned std::optional instance has no value.
			/// </returns>
			std::optional<HMODULE> LoadModule(const std::filesystem::path& moduleFilePath);

			std::optional<HMODULE> GetExistingModule(const std::filesystem::path& moduleFilePath);

			void RegisterModule(SafeModule&& loadedModule);

			/// <summary>
			/// Unloads the specified HMODULE from the application, assuming that hModule represents a valid loaded
			/// module. This is the only valid way to free a library/module; calling FreeLibrary() manually can result
			/// in inconsistent state within the DLLManager instance.
			/// 
			/// In Debug builds, the function asserts if hModule does not refer to a valid module. This might happen,
			/// for instance, if you call DLLManager::UnloadModule() multiple times.
			/// </summary>
			/// <param name="hModule">
			/// - The HMODULE representing the module/library which is to be unloaded.
			/// </param>
			void UnloadModule(const HMODULE hModule);

		private:
			template <DLLLoadType LoadType>
			std::optional<HMODULE> LoadModuleIMPL(const std::filesystem::path& moduleFilePath);

			template <DLLLoadType LoadType>
			std::optional<HMODULE> LoadModuleFromAbsoluteModulePath(const std::filesystem::path& absoluteModuleFilePath);

			template <DLLLoadType LoadType>
			std::optional<HMODULE> LoadModuleFromFileName(const std::filesystem::path& moduleFileName);

			template <DLLLoadType LoadType>
			std::optional<HMODULE> LoadModuleFromWin32(const std::filesystem::path& moduleFilePath);

		private:
			/// <summary>
			/// This map contains every DLL which was manually loaded by the DLLManager.
			/// 
			/// At no point should mModuleMap contain a SafeModule for a module which it did not load
			/// in a call to the Win32 function LoadLibraryEx().
			/// </summary>
			std::unordered_map<HMODULE, SafeModule> mModuleMap;
			mutable std::mutex mCritSection;
		};
	}
}