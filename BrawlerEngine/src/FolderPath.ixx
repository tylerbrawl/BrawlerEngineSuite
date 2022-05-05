module;
#include "DxDef.h"

export module Win32.FolderPath;

export namespace Win32
{
	namespace FolderPath
	{
		extern "C" const KNOWNFOLDERID LOCAL_APP_DATA = FOLDERID_LocalAppData;
		extern "C" const KNOWNFOLDERID WINDOWS = FOLDERID_Windows;
		extern "C" const KNOWNFOLDERID SYSTEM32 = FOLDERID_System;
		extern "C" const KNOWNFOLDERID SAVED_GAMES = FOLDERID_SavedGames;
	}
}