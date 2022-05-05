module;
#include <string>
#include <optional>
#include "DxDef.h"

export module Util.Win32;
import Win32.FolderPath;

export namespace Util
{
	namespace Win32
	{
		std::optional<std::wstring> GetKnownFolderPath(const KNOWNFOLDERID& folderID, const bool createIfNotFound = false);
	}
}