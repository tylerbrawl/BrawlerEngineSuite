module;
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <filesystem>
#include "DxDef.h"

module Brawler.DLLManager;

namespace
{
	static const std::filesystem::path APPLICATION_LIBRARY_PATH{std::filesystem::current_path() / L"Libraries" };
}

namespace Brawler
{
	namespace Win32
	{
		DLLManager::DLLManager() :
			mDirCookie(),
			mCritSection(),
			mModuleMap()
		{
			mDirCookie = AddDllDirectory(APPLICATION_LIBRARY_PATH.c_str());

			if (mDirCookie == nullptr) [[unlikely]]
				CheckHRESULT(HRESULT_FROM_WIN32(GetLastError()));
		}

		DLLManager::~DLLManager()
		{
			RemoveDllDirectory(mDirCookie);
		}

		DLLManager& DLLManager::GetInstance()
		{
			static DLLManager dllManager{};
			return dllManager;
		}
		
		bool DLLManager::LoadModule(const std::wstring_view moduleName, const ModuleSearchPath searchPath)
		{
			const DWORD dwFlags = static_cast<DWORD>(searchPath);

			{
				std::scoped_lock<std::mutex> lock{ mCritSection };

				if (mModuleMap.contains(moduleName))
					return true;

				// Try to unload any equivalent module which the OS already loaded.
				//TryFreeExistingModule(moduleName);

				const HMODULE hModule = LoadLibraryEx(moduleName.data(), nullptr, dwFlags);

				if (hModule == nullptr) [[unlikely]]
					CheckHRESULT(HRESULT_FROM_WIN32(GetLastError()));

				mModuleMap[moduleName] = SafeModule{ hModule };
			}
			
			return true;
		}

		std::optional<HMODULE> DLLManager::GetModule(const std::wstring_view moduleName) const
		{
			std::scoped_lock<std::mutex> lock{ mCritSection };
			return (mModuleMap.contains(moduleName) ? std::optional<HMODULE>{ mModuleMap.at(moduleName).get() } : std::optional<HMODULE>{});
		}
	}
}