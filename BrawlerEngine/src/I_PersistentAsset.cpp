module;
#include <utility>

module Brawler.I_PersistentAsset;
import Brawler.FilePathHash;

namespace Brawler
{
	I_PersistentAsset::I_PersistentAsset(FilePathHash&& pathHash) :
		I_Asset(std::move(pathHash))
	{}

	bool I_PersistentAsset::IsStreamable() const
	{
		return false;
	}
}