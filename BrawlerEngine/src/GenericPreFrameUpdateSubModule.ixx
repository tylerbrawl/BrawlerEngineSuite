module;
#include <mutex>
#include <vector>

export module Brawler.GenericPreFrameUpdateSubModule;
import Brawler.D3D12.BufferCopyRegion;
import Brawler.D3D12.TextureCopyRegion;
import Brawler.GenericPreFrameBufferUpdate;
import Brawler.GenericPreFrameTextureUpdate;
import Brawler.D3D12.FrameGraphBuilding;
import Brawler.D3D12.ByteAddressBufferSubAllocation;
import Brawler.D3D12.TextureCopyBufferSubAllocation;

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

		struct TextureUpdateOperation
		{
			D3D12::TextureCopyRegion DestCopyRegion;
			D3D12::TextureCopyBufferSubAllocation SrcBufferSubAllocation;
		};

		using TextureUpdatePassInfo = std::vector<TextureUpdateOperation>;

	public:
		using BufferUpdateRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, BufferUpdatePassInfo>;
		using TextureUpdateRenderPass_T = D3D12::RenderPass<D3D12::GPUCommandQueueType::COPY, TextureUpdatePassInfo>;

	public:
		GenericPreFrameUpdateSubModule() = default;

		GenericPreFrameUpdateSubModule(const GenericPreFrameUpdateSubModule& rhs) = delete;
		GenericPreFrameUpdateSubModule& operator=(const GenericPreFrameUpdateSubModule& rhs) = delete;

		GenericPreFrameUpdateSubModule(GenericPreFrameUpdateSubModule&& rhs) noexcept = delete;
		GenericPreFrameUpdateSubModule& operator=(GenericPreFrameUpdateSubModule&& rhs) noexcept = delete;

		void AddPreFrameBufferUpdate(GenericPreFrameBufferUpdate&& preFrameUpdate);
		bool HasPendingBufferUpdates() const;

		void AddPreFrameTextureUpdate(GenericPreFrameTextureUpdate&& preFrameUpdate);
		bool HasPendingTextureUpdates() const;

		BufferUpdateRenderPass_T CreateBufferUpdateRenderPass(D3D12::FrameGraphBuilder& builder);
		TextureUpdateRenderPass_T CreateTextureUpdateRenderPass(D3D12::FrameGraphBuilder& builder);

	private:
		std::vector<GenericPreFrameBufferUpdate> mPendingBufferUpdateArr;
		std::vector<GenericPreFrameTextureUpdate> mPendingTextureUpdateArr;
		mutable std::mutex mBufferUpdateCritSection;
		mutable std::mutex mTextureUpdateCritSection;
	};
}