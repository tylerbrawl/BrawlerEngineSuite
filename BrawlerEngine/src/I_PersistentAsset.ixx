module;

export module Brawler.I_PersistentAsset;
import Brawler.I_Asset;

export namespace Brawler
{
	class I_PersistentAsset : public I_Asset
	{
	protected:
		I_PersistentAsset(FilePathHash&& pathHash);

	public:
		virtual ~I_PersistentAsset() = default;

		bool IsStreamable() const override;
	};
}