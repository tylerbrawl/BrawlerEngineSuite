module;

export module Brawler.AssetManagement.EnqueuedAssetDependency;
import Brawler.AssetManagement.AssetDependency;
import Brawler.AssetManagement.AssetRequestEventHandle;

export namespace Brawler
{
	namespace AssetManagement
	{
		struct EnqueuedAssetDependency
		{
			AssetDependency Dependency;
			AssetRequestEventHandle HRequestEvent;
		};
	}
}