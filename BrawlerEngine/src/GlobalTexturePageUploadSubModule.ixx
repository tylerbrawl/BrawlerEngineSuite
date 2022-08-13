module;
#include <vector>
#include <mutex>

export module Brawler.GlobalTexturePageUploadSubModule;
import Brawler.GlobalTexturePageUploadSet;
import Brawler.IndirectionTextureUpdater;
import Brawler.D3D12.TextureCopyRegion;


export namespace Brawler
{
	class GlobalTexturePageUploadSubModule
	{
	private:
		struct GlobalTextureCopyPassInfo
		{
			
		};

	public:
		GlobalTexturePageUploadSubModule() = default;

		GlobalTexturePageUploadSubModule(const GlobalTexturePageUploadSubModule& rhs) = delete;
		GlobalTexturePageUploadSubModule& operator=(const GlobalTexturePageUploadSubModule& rhs) = delete;

		GlobalTexturePageUploadSubModule(GlobalTexturePageUploadSubModule&& rhs) noexcept = default;
		GlobalTexturePageUploadSubModule& operator=(GlobalTexturePageUploadSubModule&& rhs) noexcept = default;

		void AddPageUploadRequests(GlobalTexturePageUploadSet&& uploadSet);
		bool HasPageUploadRequests() const;



	private:
		std::vector<GlobalTexturePageUploadSet> mUploadSetArr;
		mutable std::mutex mCritSection;
	};
}