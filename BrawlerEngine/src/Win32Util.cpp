module;
#include <string>
#include <optional>
#include "DxDef.h"

module Util.Win32;

namespace Util
{
	namespace Win32
	{
		std::optional<std::wstring> GetKnownFolderPath(const KNOWNFOLDERID& folderID, const bool createIfNotFound)
		{
			PWSTR pFolderLocation = nullptr;

			DWORD dwFlags = KF_FLAG_DEFAULT;
			if (createIfNotFound)
				dwFlags |= KF_FLAG_CREATE;

			HRESULT hr = SHGetKnownFolderPath(
				folderID,
				dwFlags,
				nullptr,
				&pFolderLocation
			);

			if (FAILED(hr))
			{
				CoTaskMemFree(pFolderLocation);
				return std::optional<std::wstring>{};
			}

			std::wstring folderPath{ pFolderLocation };
			CoTaskMemFree(pFolderLocation);

			return std::optional<std::wstring>{ std::move(folderPath) };
		}
	}
}