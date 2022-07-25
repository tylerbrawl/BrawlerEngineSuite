module;
#include <memory>
#include <span>
#include <vector>

export module Brawler.VirtualTextureManagementSubModule;
import :IndirectionTextureUpdater;
import Brawler.ThreadSafeVector;
import Brawler.GlobalTextureUploadBuffer;
import Brawler.D3D12.FrameGraphBuilding;

export namespace Brawler
{
	class VirtualTextureManagementSubModule
	{
	private:
		struct ManagedUploadBuffer
		{
			std::unique_ptr<GlobalTextureUploadBuffer> ManagedBufferPtr;
			std::uint64_t SafeDeletionFrameNumber;
		};

	public:
		using IndirectionTextureUpdatePass_T = IndirectionTextureUpdater::IndirectionTextureUpdatePass_T;

	public:
		VirtualTextureManagementSubModule() = default;

		VirtualTextureManagementSubModule(const VirtualTextureManagementSubModule& rhs) = delete;
		VirtualTextureManagementSubModule& operator=(const VirtualTextureManagementSubModule& rhs) = delete;

		VirtualTextureManagementSubModule(VirtualTextureManagementSubModule&& rhs) noexcept = default;
		VirtualTextureManagementSubModule& operator=(VirtualTextureManagementSubModule&& rhs) noexcept = default;

		void CommitGlobalTextureChanges(std::unique_ptr<GlobalTextureUploadBuffer>&& preparedBufferPtr);
		bool HasCommittedGlobalTextureChanges() const;

		void PrepareForGlobalTextureUpdates();

		IndirectionTextureUpdatePass_T CreateIndirectionTextureUpdatesRenderPass(D3D12::FrameGraphBuilder& builder) const;

		void FinishGlobalTextureUpdates();

	private:
		Brawler::ThreadSafeVector<std::unique_ptr<GlobalTextureUploadBuffer>> mPreparedBufferPtrArr;
		std::vector<std::unique_ptr<GlobalTextureUploadBuffer>> mCurrentBufferArr;
		std::vector<ManagedUploadBuffer> mBuffersPendingDeletionArr;
	};
}