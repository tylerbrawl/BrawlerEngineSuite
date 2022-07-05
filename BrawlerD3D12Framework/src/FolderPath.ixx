module;
#include <ShlObj.h>
#include <KnownFolders.h>

export module Brawler.Win32.FolderPath;

export namespace Brawler
{
	namespace Win32
	{
		namespace FolderPath
		{
			extern "C" const KNOWNFOLDERID& LOCAL_APP_DATA{ FOLDERID_LocalAppData };
			extern "C" const KNOWNFOLDERID& SAVED_GAMES{ FOLDERID_SavedGames };
		}
	}
}