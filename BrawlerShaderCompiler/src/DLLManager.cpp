module;
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <filesystem>
#include <format>
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
			mDirCookie(nullptr),
			mCritSection(),
			mModuleMap()
		{
			std::error_code errorCode{};
			const bool pathExists = std::filesystem::exists(APPLICATION_LIBRARY_PATH, errorCode);

			if (errorCode) [[unlikely]]
				throw std::runtime_error{ std::format(R"(ERROR: The attempt to check whether the path "{}" exists resulted in the following error: {})", APPLICATION_LIBRARY_PATH.string(), errorCode.message()) };

			if (pathExists)
			{
				mDirCookie = AddDllDirectory(APPLICATION_LIBRARY_PATH.c_str());

				if (mDirCookie == nullptr) [[unlikely]]
					CheckHRESULT(HRESULT_FROM_WIN32(GetLastError()));
			}
		}

		DLLManager::~DLLManager()
		{
			if (mDirCookie != nullptr)
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