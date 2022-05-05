module;

export module Util.BPK;

export namespace Brawler
{
	class BPKArchiveReader;
}

export namespace Util
{
	namespace BPK
	{
		Brawler::BPKArchiveReader& GetBPKArchiveReader();
	}
}