module;
#include <vector>
#include <memory>
#include <span>
#include <mutex>

export module Brawler.GlobalTexturePageRemovalSubModule;
import Brawler.GlobalTexturePageRemovalRequest;
import Brawler.IndirectionTextureUpdater;
import Brawler.D3D12.FrameGraphBuilder;

export namespace Brawler
{
	class GlobalTexturePageRemovalSubModule
	{
	public:
		using IndirectionTextureUpdateRenderPass_T = IndirectionTextureUpdater::IndirectionTextureUpdateRenderPass_T;

	public:
		GlobalTexturePageRemovalSubModule() = default;

		GlobalTexturePageRemovalSubModule(const GlobalTexturePageRemovalSubModule& rhs) = delete;
		GlobalTexturePageRemovalSubModule& operator=(const GlobalTexturePageRemovalSubModule& rhs) = delete;

		GlobalTexturePageRemovalSubModule(GlobalTexturePageRemovalSubModule&& rhs) noexcept = default;
		GlobalTexturePageRemovalSubModule& operator=(GlobalTexturePageRemovalSubModule&& rhs) noexcept = default;

		void AddPageRemovalRequests(const std::span<std::unique_ptr<GlobalTexturePageRemovalRequest>> removalRequestSpan);
		bool HasPageRemovalRequests() const;

		std::vector<IndirectionTextureUpdateRenderPass_T> GetPageRemovalRenderPasses(D3D12::FrameGraphBuilder& builder);

	private:
		std::vector<std::unique_ptr<GlobalTexturePageRemovalRequest>> mRequestPtrArr;
		mutable std::mutex mCritSection;
	};
}