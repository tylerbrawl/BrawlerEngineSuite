module;

export module Brawler.AssetManagement.AssetRequestEventNotifier;
import Brawler.AssetManagement.AssetRequestEventHandle;

export namespace Brawler
{
	namespace AssetManagement
	{
		class AssetRequestEventNotifier
		{
		public:
			AssetRequestEventNotifier() = default;

			virtual ~AssetRequestEventNotifier() = default;

		protected:
			void MarkAssetRequestAsCompleted(AssetRequestEventHandle& hAssetRequestEvent) const;
		};
	}
}

// ------------------------------------------------------------------------------------------------------

namespace Brawler
{
	namespace AssetManagement
	{
		void AssetRequestEventNotifier::MarkAssetRequestAsCompleted(AssetRequestEventHandle& hAssetRequestEvent) const
		{
			hAssetRequestEvent.MarkAssetRequestAsCompleted();
		}
	}
}