module;
#include <mutex>
#include <unordered_map>
#include <string_view>
#include <optional>
#include <cassert>
#include <stdexcept>
#include <filesystem>
#include <format>
#include "DxDef.h"

module Brawler.Win32.DLLManager;
import Util.General;

namespace Brawler
{
	namespace Win32
	{
		template <DLLManager::DLLLoadType LoadType>
		std::optional<HMODULE> DLLManager::LoadModuleIMPL(const std::filesystem::path& moduleFilePath)
		{
			std::filesystem::path tempFilePath{ moduleFilePath.filename() };

			// The C++ specifications make it clear that path equality (comparing the string
			// representation of std::filesystem::path instances) and path equivalence (whether
			// two std::filesystem::path instances refer to the same file) are two distinct
			// ideas.
			//
			// We want the former to determine if moduleFilePath included only a file name.
			// This is provided with the operator== overload for std::filesystem::path.

			if (moduleFilePath == tempFilePath)
				return LoadModuleFromFileName<LoadType>(moduleFilePath);

			// The documentation for this function explicitly states that the functionality
			// is the same regardless of whether or moduleFilePath is an absolute or a relative
			// path. However, this is not the case for the Win32 API functions. So, if we find that
			// moduleFilePath specifies more than just a file name, then we get the absolute
			// path and use that.

			std::error_code errorCode{};
			tempFilePath = std::filesystem::absolute(moduleFilePath, errorCode);

			if (errorCode) [[unlikely]]
				return std::optional<HMODULE>{};

			const bool isAbsolutePathValid = std::filesystem::exists(tempFilePath, errorCode);

			if (errorCode || !isAbsolutePathValid) [[unlikely]]
				return std::optional<HMODULE>{};

			return LoadModuleFromAbsoluteModulePath<LoadType>(tempFilePath);
		}
		
		template <DLLManager::DLLLoadType LoadType>
		std::optional<HMODULE> DLLManager::LoadModuleFromAbsoluteModulePath(const std::filesystem::path& absoluteModuleFilePath)
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			return LoadModuleFromWin32<LoadType>(absoluteModuleFilePath);
		}

		template <DLLManager::DLLLoadType LoadType>
		std::optional<HMODULE> DLLManager::LoadModuleFromFileName(const std::filesystem::path& moduleFileName)
		{
			std::optional<HMODULE> hModule{};

			// First, we want to check the System32 directory.
			PWSTR pszSystem32DirString = nullptr;
			const HRESULT hr = SHGetKnownFolderPath(
				FOLDERID_System,
				KNOWN_FOLDER_FLAG::KF_FLAG_DEFAULT,
				nullptr,
				&pszSystem32DirString
			);

			if (FAILED(hr)) [[unlikely]]
			{
				CoTaskMemFree(pszSystem32DirString);
				Util::General::CheckHRESULT(hr);

				std::unreachable();
				return std::optional<HMODULE>{};
			}

			const std::filesystem::path system32ModulePath{ std::filesystem::path{ pszSystem32DirString } / moduleFileName };
			CoTaskMemFree(pszSystem32DirString);

			{
				std::scoped_lock<std::mutex> lock{ mCritSection };

				std::optional<HMODULE> system32Module{ LoadModuleFromWin32<LoadType>(system32ModulePath) };

				if (system32Module.has_value())
					return std::move(system32Module);

				return LoadModuleFromWin32<LoadType>(moduleFileName);
			}
		}
		
		template <DLLManager::DLLLoadType LoadType>
		std::optional<HMODULE> DLLManager::LoadModuleFromWin32(const std::filesystem::path& moduleFilePath)
		{
			// *LOCKED*
			//
			// This function is called from a locked context.

			HMODULE hModule = nullptr;

			// First, try to load an existing module with the same file path.
			{
				const bool result = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, moduleFilePath.c_str(), &hModule);

				if (result)
				{
					assert(hModule != nullptr);
					return std::optional<HMODULE>{ hModule };
				}
			}

			// If that failed, then we need to try loading the module in ourselves.
			if constexpr (LoadType == DLLLoadType::GET_EXISTING_OR_LOAD_NEW_MODULES)
			{
				hModule = LoadLibraryEx(moduleFilePath.c_str(), nullptr, 0);

				if (hModule != nullptr)
				{
					std::optional<HMODULE> returnedHModule{ hModule };
					mModuleMap.try_emplace(hModule, SafeModule{ hModule });

					return returnedHModule;
				}
			}

			return std::optional<HMODULE>{};
		}

		DLLManager::DLLManager(DLLManager&& rhs) noexcept :
			mModuleMap(),
			mCritSection()
		{
			std::scoped_lock<std::mutex> lock{ rhs.mCritSection };

			mModuleMap = std::move(rhs.mModuleMap);
		}

		DLLManager& DLLManager::operator=(DLLManager&& rhs) noexcept
		{
			// Theoretically, this could result in a deadlock. However, I would argue that
			// doing A = B and B = A concurrently across threads would be a questionable
			// design choice, anyways.
			std::scoped_lock<std::mutex, std::mutex> lock{ mCritSection, rhs.mCritSection };

			mModuleMap = std::move(rhs.mModuleMap);

			return *this;
		}

		DLLManager& DLLManager::GetInstance()
		{
			static DLLManager instance{};
			return instance;
		}
		
		std::optional<HMODULE> DLLManager::LoadModule(const std::filesystem::path& moduleFilePath)
		{
			return LoadModuleIMPL<DLLLoadType::GET_EXISTING_OR_LOAD_NEW_MODULES>(moduleFilePath);
		}

		std::optional<HMODULE> DLLManager::GetExistingModule(const std::filesystem::path& moduleFilePath)
		{
			return LoadModuleIMPL<DLLLoadType::GET_ONLY_EXISTING_MODULES>(moduleFilePath);
		}

		void DLLManager::RegisterModule(SafeModule&& loadedModule)
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			assert(!mModuleMap.contains(loadedModule.get()));
			assert(loadedModule.get() != nullptr);

			mModuleMap.try_emplace(loadedModule.get(), std::move(loadedModule));
		}

		void DLLManager::UnloadModule(const HMODULE hModule)
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };

			// If we find the HMODULE in mModuleMap, then we assume it was either loaded by
			// or assigned to the DLLManager, and just delete that entry in the map.
			//
			// Otherwise, we assume that the provided HMODULE represents a module that was
			// loaded when the process was launched. In that case, we manually call
			// FreeLibrary().

			if (mModuleMap.contains(hModule))
				mModuleMap.erase(hModule);
			else
			{
				const bool freeResult = FreeLibrary(hModule);
				assert(freeResult && "ERROR: FreeLibrary() failed in a call to DLLManager::UnloadModule()! (Did you specify an invalidated HMODULE?)");
			}
		}
	}
}