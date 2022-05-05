module;
#include <thread>

module Util.BPK;
import Brawler.Application;
import Brawler.AssetManager;

namespace Util
{
	namespace BPK
	{
		Brawler::BPKArchiveReader& GetBPKArchiveReader()
		{
			thread_local Brawler::BPKArchiveReader& bpkReader{ Brawler::GetApplication().GetAssetManager().GetBPKArchiveReader()};

			return bpkReader;
		}
	}
}