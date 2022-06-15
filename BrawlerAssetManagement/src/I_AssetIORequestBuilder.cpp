module;

module Brawler.AssetManagement.I_AssetIORequestBuilder;

namespace Brawler
{
	namespace AssetManagement
	{
		void I_AssetIORequestBuilder::SetAssetIORequestPriority(const Brawler::JobPriority priority)
		{
			mPriority = priority;
		}

		Brawler::JobPriority I_AssetIORequestBuilder::GetAssetIORequestPriority() const
		{
			return mPriority;
		}
	}
}