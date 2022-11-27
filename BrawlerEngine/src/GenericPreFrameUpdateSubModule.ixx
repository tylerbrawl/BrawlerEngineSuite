module;
#include <mutex>
#include <vector>

export module Brawler.GenericPreFrameUpdateSubModule;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.GenericPreFrameBufferUpdate;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.ByteAddressBufferSubAllocation;

export namespace Brawler
{
	class GenericPreFrameUpdateSubModule
	{
	private:
		struct BufferUpdateOperation
		{
			D3D12::BufferCopyRegion DestCopyRegion;
			D3D12::DynamicByteAddressBufferSubAllocation SrcBufferSubAllocation;
		};

		using BufferUpdatePassInfo = std::vector<BufferUpdateOperation>;

	public:
		using BufferUpdateRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, BufferUpdatePassInfo>;

	public:
		GenericPreFrameUpdateSubModule() = default;

		GenericPreFrameUpdateSubModule(const GenericPreFrameUpdateSubModule& rhs) = delete;
		GenericPreFrameUpdateSubModule& operator=(const GenericPreFrameUpdateSubModule& rhs) = delete;

		GenericPreFrameUpdateSubModule(GenericPreFrameUpdateSubModule&& rhs) noexcept = delete;
		GenericPreFrameUpdateSubModule& operator=(GenericPreFrameUpdateSubModule&& rhs) noexcept = delete;

		void AddPreFrameBufferUpdate(GenericPreFrameBufferUpdate&& preFrameUpdate);
		bool HasPendingUpdates() const;

		BufferUpdateRenderPass_T CreateBufferUpdateRenderPass(D3D12::FrameGraphBuilder& builder);

	private:
		std::vector<GenericPreFrameBufferUpdate> mPendingBufferUpdateArr;
		mutable std::mutex mBufferUpdateCritSection;
	};
}